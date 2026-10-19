#!/usr/bin/env python3
"""Check the reference header and native controls in the disposable GUI profile."""
import base64
import json
from pathlib import Path
import time
from inspect_browser import Browser

out = Path(__file__).resolve().parents[1] / 'preview'
b = Browser()
checks = []


def check(name, script, expected=True):
    result = b.js(script)
    assert result == expected, (name, result, expected)
    checks.append({'check': name, 'result': result})


def shot(name):
    result = b.command('WebDriver:TakeScreenshot', {'id': None, 'full': False, 'scroll': False})
    (out / name).write_bytes(base64.b64decode(result['value']))


def click(selector):
    result = b.command('WebDriver:FindElement', {'using': 'css selector', 'value': selector})
    element = result['value']['element-6066-11e4-a52e-4f735466cecf']
    b.command('WebDriver:ElementClick', {'id': element})


try:
    b.command('WebDriver:SetWindowRect', {'width': 1440, 'height': 900})
    b.js('SidebarController.hide(); Services.prefs.setBoolPref("sidebar.verticalTabs", false); window.resizeTo(1440, 900); window.focus(); return true;')
    b.command('Marionette:SetContext', {'value': 'content'})
    b.command('WebDriver:Navigate', {'url': 'about:newtab'})
    b.command('Marionette:SetContext', {'value': 'chrome'})
    time.sleep(1)
    check('native client-side titlebar', 'return document.documentElement.hasAttribute("customtitlebar");')
    for mode, value in [('light', 0), ('dark', 1)]:
        b.js(f'Services.prefs.setIntPref("ui.systemUsesDarkTheme", {value}); return true;')
        time.sleep(.5)
        check(mode + ' tabs below navigation', '''return document.getElementById('TabsToolbar').getBoundingClientRect().top >= document.getElementById('nav-bar').getBoundingClientRect().bottom;''')
        check(mode + ' one set of controls', '''return [...document.querySelectorAll('.titlebar-buttonbox-container')].filter(e=>e.getBoundingClientRect().width>0).length;''', 1)
        check(mode + ' traffic-light order', '''const x=s=>document.querySelector('#nav-bar '+s).getBoundingClientRect().x;
          return x('.titlebar-close') < x('.titlebar-min') && x('.titlebar-min') < x('.titlebar-max') && x('.titlebar-max') < document.getElementById('nav-bar-customization-target').getBoundingClientRect().x;''')
        check(mode + ' 14px control faces', '''return [...document.querySelectorAll('#nav-bar .titlebar-button')].filter(e=>e.getBoundingClientRect().width>0).every(e=>e.querySelector('image').getBoundingClientRect().width===14);''')
        check(mode + ' system traffic colors', '''return ['close','min','max'].map(n=>getComputedStyle(document.querySelector('#nav-bar .titlebar-'+n)).getPropertyValue('--se-light').trim());''', ['#ff625a','#ffbd44','#30c84b'])
        shot('header-' + mode + '.png')
    # Exercise the native maximize and restore command paths in this test window.
    click('#nav-bar .titlebar-max')
    time.sleep(.7)
    check('maximize command works', 'return window.windowState === window.STATE_MAXIMIZED;')
    check('restore replaces maximize', '''return document.querySelector('#nav-bar .titlebar-restore').getBoundingClientRect().width > 0 && document.querySelector('#nav-bar .titlebar-max').getBoundingClientRect().width === 0;''')
    click('#nav-bar .titlebar-restore')
    time.sleep(.7)
    check('restore command works', 'return window.windowState === window.STATE_NORMAL;')
    b.js('window.sequoiaCommands=[]; document.addEventListener("command", event => window.sequoiaCommands.push(event.target.id), {capture:true}); return true;')
    click('#nav-bar .titlebar-min')
    time.sleep(.7)
    check('minimize dispatches native command', 'return window.sequoiaCommands.includes("cmd_minimizeWindow");')
    b.command('WebDriver:SetWindowRect', {'width': 800, 'height': 700})
    # Narrow window and multiple tabs must preserve address input and controls.
    b.js('window.resizeTo(800, 700); for(let i=0;i<5;i++)gBrowser.addTab("about:blank", {triggeringPrincipal:Services.scriptSecurityManager.getSystemPrincipal()}); return true;')
    time.sleep(.5)
    check('narrow test uses a narrow viewport', 'return window.innerWidth <= 800;')
    check('narrow window keeps usable address field', 'return document.getElementById("urlbar").getBoundingClientRect().width > 100;')
    check('multiple tabs retain individual hit targets', 'return [...gBrowser.tabs].every(t=>t.getBoundingClientRect().width > 60);')
    shot('header-narrow.png')
    b.js('gBrowser.selectedTab=gBrowser.tabs[0]; for(const tab of [...gBrowser.tabs].slice(1))gBrowser.removeTab(tab,{animate:false}); return true;')
    b.command('WebDriver:SetWindowRect', {'width': 1440, 'height': 900})
    (out / 'header-validation.json').write_text(json.dumps(checks, indent=2) + '\n')
    print(f'PASS: {len(checks)} header checks including native maximize, restore, and minimize')
finally:
    b.close()
