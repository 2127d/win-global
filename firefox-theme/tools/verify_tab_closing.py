#!/usr/bin/env python3
"""Animated tab close + session restore regression, disposable profile only.

Use port 2832 with browser.startup.page=3 and ui.prefersReducedMotion=0.
Run once, gracefully quit/relaunch that profile, then run with --restored.
The first run replaces the test window's tabs with local fixture pages.
"""
import argparse
import json
from pathlib import Path
import tempfile
import time
from inspect_browser import Browser

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--restored', action='store_true')
args = parser.parse_args()
report = Path(__file__).resolve().parents[1] / 'preview/tab-closing-validation.json'
b = Browser(2832)


def wait_for(script, label):
    for _ in range(40):
        if b.js(script):
            return
        time.sleep(.1)
    raise AssertionError(label)


def current_urls():
    return b.js('return [...gBrowser.tabs].map(t => t.linkedBrowser.currentURI.spec);')


def move_off_tabs():
    b.command('WebDriver:PerformActions', {'actions': [{'type': 'pointer', 'id': 'mouse',
        'parameters': {'pointerType': 'mouse'}, 'actions': [
            {'type': 'pointerMove', 'duration': 0, 'origin': 'viewport', 'x': 400, 'y': 300}]}]})


try:
    if args.restored:
        result = json.loads(report.read_text())
        wait_for('return gBrowser.tabs.length === 2;', 'Expected exactly the two retained tabs after restart')
        assert sorted(current_urls()) == sorted(result['retained_urls']), current_urls()
        assert b.js('return !document.querySelector(".tabbrowser-tab[closing]");')
        result['restart_restores_only_retained_tabs'] = True
        report.write_text(json.dumps(result, indent=2) + '\n')
        print('PASS: closed tabs stay closed across a real Firefox restart')
    else:
        fixture = Path(tempfile.mkdtemp(prefix='sequoia-tab-fixtures-'))
        urls = {}
        for name in ['keep-a', 'keep-b', 'close-mouse', 'close-keyboard']:
            page = fixture / (name + '.html')
            page.write_text(f'<title>{name}</title><h1>{name}</h1>')
            urls[name] = page.as_uri()
        b.command('WebDriver:SetWindowRect', {'width': 1440, 'height': 900})
        b.js('Services.prefs.setBoolPref("sidebar.verticalTabs", false); return true;')
        b.js('for(const t of [...gBrowser.tabs]) if(t!==gBrowser.selectedTab) gBrowser.removeTab(t,{animate:false}); return true;')
        wait_for('return gBrowser.tabs.length===1;', 'Test setup should retain one tab')
        b.command('Marionette:SetContext', {'value': 'content'})
        b.command('WebDriver:Navigate', {'url': urls['keep-a']})
        b.command('Marionette:SetContext', {'value': 'chrome'})
        b.js('''for (const [name,url] of Object.entries(''' + json.dumps(urls) + ''')) {
          if (name === 'keep-a') continue;
          const t=gBrowser.addTab(url,{triggeringPrincipal:Services.scriptSecurityManager.getSystemPrincipal()});
          t.setAttribute('sequoia-test-id',name);
        } return true;''')
        wait_for('return gBrowser.tabs.length===4 && [...gBrowser.tabs].every(t=>t._fullyOpen && !t.hasAttribute("busy"));', 'Tabs should finish their opening animations')
        initial_width = b.js('return gBrowser.tabs[0].getBoundingClientRect().width;')
        b.js('window.sequoiaClosedMouse=document.querySelector("[sequoia-test-id=close-mouse]"); return true;')
        button = b.command('WebDriver:FindElement', {'using': 'css selector', 'value': '[sequoia-test-id=close-mouse] .tab-close-button'})['value']
        b.command('WebDriver:ElementClick', {'id': button['element-6066-11e4-a52e-4f735466cecf']})
        wait_for('return !window.sequoiaClosedMouse.isConnected && gBrowser.tabs.length===3;', 'Mouse-close must actually remove the tab')
        # Firefox briefly freezes sibling widths under the pointer for repeated
        # mouse closes. Moving into the page releases that native sizing lock.
        move_off_tabs()
        wait_for(f'return gBrowser.tabs[0].getBoundingClientRect().width > {initial_width + 30};', 'Remaining tabs should reclaim the closed tab width')
        mouse_width = b.js('return gBrowser.tabs[0].getBoundingClientRect().width;')
        b.js('window.sequoiaClosedKey=document.querySelector("[sequoia-test-id=close-keyboard]"); gBrowser.selectedTab=window.sequoiaClosedKey; gBrowser.selectedBrowser.focus(); return true;')
        b.command('WebDriver:PerformActions', {'actions': [{'type': 'key', 'id': 'keyboard', 'actions': [
            {'type': 'keyDown', 'value': '\ue009'}, {'type': 'keyDown', 'value': 'w'},
            {'type': 'keyUp', 'value': 'w'}, {'type': 'keyUp', 'value': '\ue009'}]}]})
        wait_for('return !window.sequoiaClosedKey.isConnected && gBrowser.tabs.length===2;', 'Ctrl+W must actually remove the tab')
        wait_for(f'return gBrowser.tabs[0].getBoundingClientRect().width > {mouse_width + 30};', 'Remaining tabs should expand again after Ctrl+W')
        retained = [urls['keep-a'], urls['keep-b']]
        assert sorted(current_urls()) == sorted(retained)
        b.js('return ChromeUtils.importESModule("moz-src:///browser/components/sessionstore/TabStateFlusher.sys.mjs").TabStateFlusher.flushWindow(window).then(()=>true);')
        state = b.js('const s=SessionStore.getWindowState(window); return (typeof s==="string" ? JSON.parse(s) : s).windows[0].tabs.map(t=>t.entries[t.index-1].url);')
        assert sorted(state) == sorted(retained), state
        result = {
            'firefox': b.js('return Services.appinfo.version;'),
            'mouse_close_removes_tab': True, 'keyboard_close_removes_tab': True,
            'no_stuck_closing_tabs': b.js('return !document.querySelector(".tabbrowser-tab[closing]");'),
            'remaining_tab_widths': [initial_width, mouse_width, b.js('return gBrowser.tabs[0].getBoundingClientRect().width;')],
            'session_contains_only_retained_tabs': True, 'retained_urls': retained,
            'restart_restores_only_retained_tabs': False,
        }
        report.write_text(json.dumps(result, indent=2) + '\n')
        print('PASS: mouse close, Ctrl+W, reclaimed width, and saved-session contents')
finally:
    b.close()
