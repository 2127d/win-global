#!/usr/bin/env python3
"""Check the same supported-widget model used by Plasma's Add Widgets picker."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.sequoia.digitalclock'


def child():
    from PySide6.QtWidgets import QApplication
    from PySide6.QtQml import QQmlEngine, QQmlComponent
    from PySide6.QtCore import QUrl
    app = QApplication([])
    engine = QQmlEngine()
    component = QQmlComponent(engine)
    component.setData(b'import org.kde.plasma.private.shell\nWidgetExplorer {}', QUrl())
    explorer = component.create()
    assert explorer, component.errors()
    model = explorer.property('widgetsModel')
    roles = {bytes(name).decode(): role for role, name in model.roleNames().items()}
    for row in range(model.rowCount()):
        index = model.index(row, 0)
        if model.data(index, roles['pluginName']) != PLUGIN:
            continue
        supported = model.data(index, roles['isSupported'])
        reason = model.data(index, roles['unsupportedMessage'])
        assert supported, reason
        print('PASS: Add Widgets reports Sequoia Digital Clock as supported.', flush=True)
        return
    raise AssertionError('Sequoia Digital Clock is absent from Add Widgets')


def main():
    with tempfile.TemporaryDirectory(prefix='sequoia-clock-support-', ignore_cleanup_errors=True) as temporary:
        stage = Path(temporary)
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                   QT_NO_XDG_DESKTOP_PORTAL='1', QT_QPA_PLATFORMTHEME='generic')
        for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH', 'QT_PLUGIN_PATH'):
            env.pop(key, None)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = stage / kind.lower()
            path.mkdir(mode=0o700)
            env[f'XDG_{kind}_' + ('DIR' if kind == 'RUNTIME' else 'HOME')] = str(path)
        shutil.copytree(ROOT / f'widgets/plasmoids/{PLUGIN}', stage / f'data/plasma/plasmoids/{PLUGIN}')
        subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()), '--child'],
                       env=env, check=True, timeout=20)


if __name__ == '__main__':
    child() if '--child' in sys.argv else main()
