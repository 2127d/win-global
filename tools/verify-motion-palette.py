#!/usr/bin/env python3
"""Load staged motion effects in private KWin and exercise real windows/popups."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--stage', type=Path, default=ROOT / '.staging/motion-approved')
parser.add_argument('--child', type=Path)
args = parser.parse_args()
STAGE = args.stage.resolve()


def child(base):
    from PySide6.QtCore import QTimer
    from PySide6.QtWidgets import QApplication, QWidget, QMenu
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
    log_path = base / 'kwin.log'
    with log_path.open('w') as log:
        compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '720',
            '--no-lockscreen', '--no-global-shortcuts', '--socket', 'motion-test'], env=env, stdout=log, stderr=log)
    def dbus(method, name):
        return subprocess.check_output(['qdbus-qt6', 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.' + method, name], text=True).strip()
    try:
        for _ in range(100):
            if (base / 'runtime/motion-test').exists():
                break
            if compositor.poll() is not None:
                raise RuntimeError(log_path.read_text())
            time.sleep(.1)
        os.environ.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='motion-test')
        app = QApplication([])
        for name in json.loads((STAGE / 'plan.json').read_text())['effects']:
            assert dbus('isEffectLoaded', 'sequoia-motion-' + name) == 'true', name
        info = dbus('supportInformation', 'overview')
        assert 'animationDuration: 200' in info, info
        assert dbus('isEffectLoaded', 'slidingpopups') == 'true', 'Panel slide effect was disabled'
        slide = dbus('supportInformation', 'slidingpopups')
        assert 'slideInDuration: 200' in slide and 'slideOutDuration: 200' in slide, slide
        window = QWidget()
        window.setWindowTitle('Motion verification')
        window.resize(500, 350)
        menu = QMenu(window)
        menu.addAction('Motion verification')
        window.show()
        QTimer.singleShot(500, lambda: menu.popup(window.mapToGlobal(window.rect().center())))
        QTimer.singleShot(850, menu.close)
        QTimer.singleShot(1100, window.showMaximized)
        QTimer.singleShot(1550, window.showNormal)
        QTimer.singleShot(1950, window.showFullScreen)
        QTimer.singleShot(2400, window.showNormal)
        QTimer.singleShot(2800, window.showMinimized)
        QTimer.singleShot(3250, window.showNormal)
        QTimer.singleShot(3700, window.close)
        QTimer.singleShot(4200, app.quit)
        app.setQuitOnLastWindowClosed(False)
        app.exec()
        # Also load actual updated Plasma components and the installed fork QML.
        shell_env = dict(os.environ, QML_IMPORT_PATH=str(base / 'data/sequoia-eleven/motion/qml'),
            QT_QUICK_BACKEND='software')
        with (base / 'plasma.log').open('w') as log:
            shell = subprocess.Popen(['plasmawindowed', 'org.sequoia.icontasks.animated'], env=shell_env, stdout=log, stderr=log)
            try:
                time.sleep(3)
                assert shell.poll() is None, 'Dock applet exited while loading'
            finally:
                shell.terminate()
                shell.wait(timeout=5)
        errors = []
        for logfile in (log_path, base / 'plasma.log'):
            text = logfile.read_text()
            errors += [line for line in text.splitlines() if any(error in line for error in
                ('ReferenceError', 'TypeError', 'SyntaxError', 'Cannot assign', 'is not a type', 'Error loading QML'))]
        assert not errors, '\n'.join(errors)
        from PySide6.QtCore import QObject, QUrl
        from PySide6.QtQml import QQmlComponent, QQmlEngine
        engine = QQmlEngine()
        engine.addImportPath(str(base / 'data/sequoia-eleven/motion/qml'))
        component = QQmlComponent(engine)
        component.setData(b'import QtQuick; import org.kde.plasma.components as PC; PC.Switch { }', QUrl())
        switch = component.create()
        assert switch, component.errors()
        durations = [o.property('duration') for o in switch.findChildren(QObject) if o.property('duration') is not None]
        assert 200 in durations, durations
        print('PASS: six real KWin effects load; panel sliding and Overview report 200 ms; open/close/menu/maximize/fullscreen/minimize; Plasma dock loads without QML errors')
    finally:
        compositor.terminate()
        compositor.wait(timeout=8)
        shutil.copy2(log_path, ROOT / '.staging/motion-kwin-test.log')
        if (base / 'plasma.log').exists():
            shutil.copy2(base / 'plasma.log', ROOT / '.staging/motion-plasma-test.log')


if args.child:
    child(args.child)
else:
    with tempfile.TemporaryDirectory(prefix='sequoia-motion-native-') as directory:
        base = Path(directory)
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_NO_XDG_DESKTOP_PORTAL='1')
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = base / kind.lower()
            path.mkdir(mode=0o700)
            env[f'XDG_{kind}_' + ('DIR' if kind == 'RUNTIME' else 'HOME')] = str(path)
        for key in ('QML_IMPORT_PATH', 'QT_PLUGIN_PATH', 'LD_PRELOAD'):
            env.pop(key, None)
        shutil.copytree(STAGE / 'data', base / 'data', dirs_exist_ok=True, symlinks=True)
        # Avoid activating first-run applications inside the test compositor.
        (base / 'config/plasma-welcomerc').write_text('[General]\nShouldShow=false\n')
        plan = json.loads((STAGE / 'plan.json').read_text())
        for file, group, key, value in plan['settings']:
            subprocess.run(['kwriteconfig6', '--file', str(base / 'config' / file), '--group', group, '--key', key, value], check=True)
        subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()), '--stage', str(STAGE), '--child', str(base)],
            env=env, check=True, timeout=45)
