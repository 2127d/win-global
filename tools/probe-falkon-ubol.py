#!/usr/bin/env python3
"""Test an unpacked uBlock Origin Lite release against the installed Qt engine.

Requires PySide6 with Qt WebEngine 6.10+. Uses a temporary profile and localhost
HTTP fixtures. Returns 1 if blocking or required browser APIs fail, 2 on timeout.
"""
import argparse
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
from pathlib import Path
import tempfile
import threading


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extension', type=Path, required=True)
    args = parser.parse_args()
    extension = args.extension.resolve()
    manifest = json.loads((extension / 'manifest.json').read_text())
    # This fixture must match an enabled stock EasyList rule, not a test rule
    # injected into the extension. Fail if a future package changes the rule.
    easylist = next(r for r in manifest['declarative_net_request']['rule_resources']
                    if r['id'] == 'easylist' and r['enabled'])
    rules = json.loads((extension / easylist['path'].lstrip('/')).read_text())
    assert any(r['action']['type'] == 'block'
               and r['condition'] == {'resourceTypes': ['script'], 'urlFilter': '/ads/cbr.js'}
               for r in rules), 'Release no longer contains the expected fixture rule'

    with tempfile.TemporaryDirectory(prefix='falkon-ubol-test-') as tmp:
        for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
            path = Path(tmp) / kind.lower()
            path.mkdir(mode=0o700)
            key = f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'
            os.environ[key] = str(path)
        os.environ['QT_QPA_PLATFORM'] = 'offscreen'
        os.environ['QTWEBENGINE_CHROMIUM_FLAGS'] = '--disable-gpu --no-proxy-server --enable-logging=stderr'

        from PySide6.QtCore import QTimer, QUrl, qVersion
        from PySide6.QtWidgets import QApplication
        from PySide6.QtWebEngineCore import QWebEnginePage, QWebEngineProfile, QWebEngineExtensionInfo
        import shiboken6

        requests = []

        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                requests.append(self.path)
                self.send_response(200)
                self.send_header('Cache-Control', 'no-store')
                self.send_header('Content-Type', 'text/html' if self.path == '/' else 'text/javascript')
                self.end_headers()
                self.wfile.write({
                    '/': b'<script>window.fixture={};</script><script src="/allowed.js"></script>'
                         b'<script src="/ads/cbr.js"></script><p>Local blocking fixture</p>',
                    '/allowed.js': b'window.fixture.allowed=true;',
                    '/ads/cbr.js': b'window.fixture.ad=true;',
                }.get(self.path, b''))

            def log_message(self, *args):
                pass

        class Page(QWebEnginePage):
            def javaScriptConsoleMessage(self, level, message, line, source):
                print(f'JS: {message} ({source}:{line})', flush=True)

        server = HTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        app = QApplication([])
        app.setQuitOnLastWindowClosed(False)
        # Changing persistentStoragePath after construction can crash Qt's
        # extension preferences. The isolated XDG directories suffice here.
        profile = QWebEngineProfile('ubol-probe', app)
        manager = profile.extensionManager()
        page = Page(profile, app)
        panel = Page(profile, app)
        url = QUrl(f'http://127.0.0.1:{server.server_port}/')
        state = {'phase': 'baseline', 'blocked': False}

        def inspected(raw):
            data = json.loads(raw or '{}')
            print('EXTENSION APIS: ' + json.dumps(data), flush=True)
            success = state['blocked'] and not data.get('missing', ['unknown'])
            print('PASS' if success else 'FAIL: Lite is not ready for stock integration', flush=True)
            app.exit(0 if success else 1)

        def panel_loaded(ok):
            if not ok:
                print('FAIL: popup could not load', flush=True)
                app.exit(1)
                return
            panel.runJavaScript("""JSON.stringify({
                available: Object.keys(chrome),
                missing: ['permissions','scripting','commands','action'].filter(k => !chrome[k])
            })""", inspected)

        def fixture_result(raw):
            data = json.loads(raw or '{}')
            print(state['phase'].upper() + ': ' + json.dumps(data), flush=True)
            if state['phase'] == 'baseline':
                if data != {'allowed': True, 'ad': True}:
                    print('FAIL: unfiltered fixture did not load', flush=True)
                    app.exit(1)
                    return
                state['phase'] = 'filtered'
                requests.clear()
                manager.loadExtension(str(extension))
            else:
                state['blocked'] = (data == {'allowed': True} and '/ads/cbr.js' not in requests)
                print('SERVER REQUESTS: ' + json.dumps(requests), flush=True)
                panel.load(state['popup'])

        def page_loaded(ok):
            if not ok:
                app.exit(1)
                return
            page.runJavaScript('JSON.stringify(window.fixture)', fixture_result)

        def loaded(info):
            info = QWebEngineExtensionInfo(info)
            print(f'LOAD: {info.isLoaded()}, {info.error()}', flush=True)
            if not info.isLoaded():
                app.exit(1)
                return
            state['popup'] = info.actionPopupUrl()
            manager.setExtensionEnabled(info, True)
            print('ENABLED: ' + str(any(e.id() == info.id() and e.isEnabled()
                                       for e in manager.extensions())), flush=True)
            QTimer.singleShot(5000, lambda: page.load(url))

        manager.loadFinished.connect(loaded)
        page.loadFinished.connect(page_loaded)
        panel.loadFinished.connect(panel_loaded)
        print(f'Qt {qVersion()}, Lite {manifest["version"]}', flush=True)
        page.load(url)
        QTimer.singleShot(25000, lambda: app.exit(2))
        status = app.exec()
        shiboken6.delete(panel)
        shiboken6.delete(page)
        shiboken6.delete(profile)
        server.shutdown()
        server.server_close()
        thread.join()
        return status


if __name__ == '__main__':
    raise SystemExit(main())
