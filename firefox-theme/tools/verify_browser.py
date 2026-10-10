#!/usr/bin/env python3
"""Validate a separate running Firefox test profile; never use a personal profile.

Uses the local automation connection described in inspect_browser.py.
Writes screenshots and a JSON report to preview/.
"""
import base64
import json
from pathlib import Path
import time
from inspect_browser import Browser

OUT = Path(__file__).resolve().parents[1] / 'preview'
OUT.mkdir(exist_ok=True)
b = Browser()
checks = []


def check(label, script, expected):
    result = b.js(script)
    assert result == expected, (label, result, expected)
    checks.append({'check': label, 'result': result})


def screenshot(name):
    data = b.command('WebDriver:TakeScreenshot', {'id': None, 'full': False, 'scroll': False})
    (OUT / name).write_bytes(base64.b64decode(data['value']))


try:
    b.command('WebDriver:SetWindowRect', {'width': 1440, 'height': 1000})
    b.js('PanelUI.hide(); window.resizeTo(1440, 1000); SidebarController.show("viewHistorySidebar"); return true;')
    time.sleep(1)
    for mode, value, color in [('dark', 1, 'rgb(37, 38, 41)'), ('light', 0, 'rgb(242, 243, 245)')]:
        b.js(f'Services.prefs.setIntPref("ui.systemUsesDarkTheme", {value}); return true;')
        time.sleep(.6)
        check(mode + ' floating toolbar', 'return getComputedStyle(document.getElementById("nav-bar")).backgroundColor;', 'rgba(0, 0, 0, 0)')
        check(mode + ' sidebar corners', 'return getComputedStyle(document.getElementById("sidebar-box")).borderRadius;', '14px')
        check(mode + ' sidebar document', 'return getComputedStyle(document.getElementById("sidebar").contentDocument.body).backgroundColor;', color)
        check(mode + ' address bar', 'return getComputedStyle(document.querySelector(".urlbar-background, #urlbar-background")).borderRadius;', '999px')
        b.js('PanelUI.show(); return true;')
        time.sleep(.3)
        check(mode + ' app menu background', '''const p=document.getElementById('appMenu-popup');
          return getComputedStyle(p.shadowRoot.querySelector('[part~="content"]')).backgroundColor;''', color)
        check(mode + ' app menu corners', '''const p=document.getElementById('appMenu-popup');
          return getComputedStyle(p.shadowRoot.querySelector('[part~="content"]')).borderRadius;''', '14px')
        b.js('PanelUI.hide(); document.getElementById("toolbar-context-menu").openPopupAtScreen(200, 150, true); return true;')
        time.sleep(.2)
        check(mode + ' context menu background', '''const p=document.getElementById('toolbar-context-menu');
          return getComputedStyle(p.shadowRoot.querySelector('[part~="content"]')).backgroundColor;''', color)
        b.js('document.getElementById("toolbar-context-menu").hidePopup(); return true;')
        screenshot(mode + '.png')
    b.js('Services.prefs.setBoolPref("sidebar.verticalTabs", true); return true;')
    time.sleep(1)
    check('vertical tabs visible', 'return document.getElementById("tabbrowser-tabs").getBoundingClientRect().height > 40;', True)
    screenshot('vertical-tabs.png')
    b.js('Services.prefs.setBoolPref("sidebar.verticalTabs", false); document.documentElement.setAttribute("inDOMFullscreen", "true"); return true;')
    check('fullscreen content has no inset', 'return getComputedStyle(document.getElementById("tabbrowser-tabbox")).margin;', '0px')
    b.js('document.documentElement.removeAttribute("inDOMFullscreen"); return true;')
    b.command('Marionette:SetContext', {'value': 'content'})
    for page in ['about:preferences', 'about:addons', 'about:newtab', 'about:logins', 'about:downloads']:
        b.command('WebDriver:Navigate', {'url': page})
        check(page + ' canvas', 'return getComputedStyle(document.documentElement).backgroundColor;', 'rgb(231, 235, 242)')
    b.command('WebDriver:Navigate', {'url': 'data:text/html,<body style="background:rgb(12,34,56)">Website color test</body>'})
    check('ordinary websites retain colors', 'return getComputedStyle(document.body).backgroundColor;', 'rgb(12, 34, 56)')
    b.command('WebDriver:Navigate', {'url': 'about:preferences'})
    b.command('Marionette:SetContext', {'value': 'chrome'})
    errors = b.js(r'''return Services.console.getMessageArray()
      .filter(m => m instanceof Ci.nsIScriptError && /sequoia\/.*\.css/.test(m.sourceName))
      .map(m => ({file:m.sourceName, line:m.lineNumber, message:m.errorMessage}));''')
    checks.append({'check': 'Firefox CSS diagnostics', 'result': errors})
    assert not errors, errors
    (OUT / 'validation.json').write_text(json.dumps({'firefox': b.js('return Services.appinfo.version;'), 'checks': checks}, indent=2) + '\n')
    print(f'PASS: {len(checks)} checks; screenshots and report in {OUT}')
finally:
    b.close()
