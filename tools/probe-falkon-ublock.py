#!/usr/bin/env python3
"""Probe uBlock's manifest compatibility with the installed Qt WebEngine.

Requires PySide6 with Qt WebEngine 6.10+. This is a manifest admission check,
not a packaged uBlock build or a filtering test. No extension is enabled.
"""
import json
import os
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'apps/falkon1/uBlock'


def main():
    with tempfile.TemporaryDirectory(prefix='falkon-ublock-probe-') as tmp:
        base = Path(tmp)
        for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
            path = base / kind.lower()
            path.mkdir(mode=0o700)
            key = f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'
            os.environ[key] = str(path)
        os.environ['QT_QPA_PLATFORM'] = 'offscreen'
        os.environ['QTWEBENGINE_CHROMIUM_FLAGS'] = '--disable-gpu'

        from PySide6.QtWidgets import QApplication
        from PySide6.QtCore import QTimer, qVersion
        from PySide6.QtWebEngineCore import QWebEngineProfile

        extension = base / 'ublock'
        shutil.copytree(SOURCE / 'src', extension)
        for folder in ('common', 'chromium'):
            for path in (SOURCE / 'platform' / folder).iterdir():
                if path.suffix == '.js':
                    shutil.copy2(path, extension / 'js' / path.name)
                elif path.suffix in ('.json', '.html'):
                    shutil.copy2(path, extension / path.name)
        control = base / 'mv3-control'
        control.mkdir()
        (control / 'manifest.json').write_text(json.dumps({
            'manifest_version': 3, 'name': 'Qt compatibility control', 'version': '1.0'
        }))

        app = QApplication([])
        profile = QWebEngineProfile('ublock-probe', app)
        profile.setPersistentStoragePath(str(base / 'profile'))
        profile.setCachePath(str(base / 'cache'))
        manager = profile.extensionManager()
        results = []

        def loaded(info):
            result = {'fixture': 'ublock' if not results else 'mv3-control',
                      'loaded': info.isLoaded(), 'error': info.error()}
            print(json.dumps(result), flush=True)
            results.append(result)
            if len(results) == 1:
                QTimer.singleShot(0, lambda: manager.loadExtension(str(control)))
            else:
                app.exit(0 if results[1]['loaded'] else 2)

        manager.loadFinished.connect(loaded)
        print('Qt runtime ' + qVersion(), flush=True)
        QTimer.singleShot(0, lambda: manager.loadExtension(str(extension)))
        QTimer.singleShot(20000, lambda: app.exit(2))
        return app.exec()


if __name__ == '__main__':
    raise SystemExit(main())
