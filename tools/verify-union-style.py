#!/usr/bin/env python3
"""Test Sequoia CSS in the installed Union engine using disposable XDG state."""
import argparse
import configparser
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
STYLE = ROOT / 'themes/union/sequoia-popup-prototype'


def child(kind, output, mode):
    from PySide6.QtCore import Qt, QUrl
    from PySide6.QtWidgets import (QApplication, QWidget, QVBoxLayout, QHBoxLayout,
                                  QGroupBox, QPushButton, QLineEdit, QSlider, QLabel)
    from PySide6.QtTest import QTest
    app = QApplication([])
    assert app.style().objectName().lower() == 'union', app.style().objectName()
    if kind == 'quick':
        from PySide6.QtQuick import QQuickItem, QQuickWindow
        from PySide6.QtQml import QQmlApplicationEngine
        engine = QQmlApplicationEngine()
        engine.load(QUrl.fromLocalFile(str(ROOT / 'themes/union/Probe.qml')))
        assert engine.rootObjects(), 'Union QML failed to load'
        window = engine.rootObjects()[0]
        QTest.qWait(700)
        report = window.property('report').toVariant()
        action = window.findChild(QQuickItem, 'action')
        action.forceActiveFocus()
        QTest.keyClick(window, Qt.Key_Space)
        assert window.property('activations') == 1, 'Keyboard action failed'
        disabled = window.findChild(QQuickItem, 'disabled')
        point = disabled.mapToScene(disabled.boundingRect().center()).toPoint()
        QTest.mouseClick(window, Qt.LeftButton, pos=point)
        assert window.property('activations') == 1, 'Disabled button activated'
        QTest.qWait(250)
        shot = window.grabWindow()
        assert not shot.isNull(), 'Empty Quick screenshot'
        shot.save(str(output / f'union-quick-{mode}.png'))
        assert report['cardRadius'] == 18 and report['buttonRadius'] == 10, report
        assert report['cardAlpha'] == 1 and abs(report['glassAlpha'] - 0.72) < .01, report
        assert abs(report['windowAlpha'] - 0.72) < .01, report
        assert report['shadowAlpha'] > 0 and report['shadowBlur'] > 0, report
        report['keyboard_and_disabled_controls'] = True
        # Report alpha from the actual window buffer, not only CSS properties.
        report['window_buffer_alpha'] = shot.pixelColor(2, 2).alphaF()
    else:
        window = QWidget()
        window.setWindowTitle('Sequoia · Union Qt Widgets')
        window.resize(660, 430)
        layout = QVBoxLayout(window)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.addWidget(QLabel('Sequoia / Union — native Qt Widgets'))
        card = QGroupBox('Content island')
        layout.addWidget(card)
        content = QVBoxLayout(card)
        buttons = QHBoxLayout()
        action = QPushButton('Action')
        selected = QPushButton('Selected'); selected.setCheckable(True); selected.setChecked(True)
        disabled = QPushButton('Unavailable'); disabled.setEnabled(False)
        for button in (action, selected, disabled): buttons.addWidget(button)
        content.addLayout(buttons)
        search = QLineEdit(); search.setPlaceholderText('Search desktop surfaces'); content.addWidget(search)
        slider = QSlider(Qt.Horizontal); slider.setValue(65); content.addWidget(slider)
        layout.addWidget(QLabel('The application still owns layout and window translucency.'))
        count = []
        action.clicked.connect(lambda: count.append(1))
        disabled.clicked.connect(lambda: count.append(2))
        window.show(); QTest.qWait(600)
        action.setFocus(); QTest.keyClick(action, Qt.Key_Space)
        QTest.mouseClick(disabled, Qt.LeftButton)
        assert count == [1], count
        shot = window.grab().toImage()
        shot.save(str(output / f'union-widgets-{mode}.png'))
        # CSS corner shape must actually reach the QStyle painter: compare an
        # opaque center/edge sample with the corner's outside material.
        button_image = action.grab().toImage()
        report = {'style': app.style().objectName(), 'keyboard_and_disabled_controls': True,
                  'button_corner': button_image.pixelColor(1, 1).name(),
                  'button_face': button_image.pixelColor(button_image.width() // 2, 5).name(),
                  'automatic_window_translucency': window.testAttribute(Qt.WA_TranslucentBackground)}
        assert report['button_corner'] != report['button_face'], report
    (output / f'union-{kind}-{mode}.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: Union {kind} {mode}: ' + json.dumps(report), flush=True)
    window.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'preview/union')
    parser.add_argument('--child', choices=['quick', 'widgets'])
    parser.add_argument('--mode', choices=['light', 'dark'], default='light')
    parser.add_argument('--native', action='store_true', help='Use a private Wayland compositor and real scene graph')
    parser.add_argument('--session', choices=['quick', 'widgets'], help=argparse.SUPPRESS)
    args = parser.parse_args()
    args.output = args.output.resolve(); args.output.mkdir(parents=True, exist_ok=True)
    if args.session:
        runtime = Path(os.environ['XDG_RUNTIME_DIR'])
        with (args.output / f'kwin-{args.session}-{args.mode}.log').open('w') as log:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1000', '--height', '700',
                '--no-lockscreen', '--no-global-shortcuts', '--socket', 'sequoia-union'], stdout=log, stderr=log)
            try:
                for _ in range(100):
                    if (runtime / 'sequoia-union').exists(): break
                    if compositor.poll() is not None: raise RuntimeError('Private compositor exited')
                    time.sleep(.1)
                else: raise RuntimeError('Private compositor did not start')
                env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-union',
                           QT_QUICK_BACKEND='rhi', WAYLAND_DEBUG='client')
                result = subprocess.run([sys.executable, __file__, '--child', args.session,
                    '--mode', args.mode, '--output', str(args.output)], env=env, capture_output=True, text=True, timeout=30)
                protocol = result.stderr
                blur = [line for line in protocol.splitlines() if
                        ('ext_background_effect_surface_v1#' in line and '.set_blur_region(' in line and 'nil' not in line)
                        or ('org_kde_kwin_blur#' in line and '.commit(' in line)]
                path = args.output / f'union-{args.session}-{args.mode}.json'
                if path.exists() and result.returncode == 0:
                    report = json.loads(path.read_text())
                    report.update(native_wayland=True, compositor_blur_requested=bool(blur))
                    path.write_text(json.dumps(report, indent=2) + '\n')
                    print(f'PASS: native {args.session} {args.mode}; blur requested: {bool(blur)}')
                print(result.stdout, end=''); print(protocol, end='', file=sys.stderr)
                raise SystemExit(result.returncode)
            finally:
                compositor.terminate()
                try: compositor.wait(timeout=5)
                except subprocess.TimeoutExpired: compositor.kill(); compositor.wait()
    if args.child:
        child(args.child, args.output, args.mode)
        return
    from popup_components import sync, SOURCE
    sync(check=True)
    tokens = json.loads((SOURCE / 'tokens.json').read_text())
    for mode in ('light', 'dark'):
        for kind in ('quick', 'widgets'):
            with tempfile.TemporaryDirectory(prefix='sequoia-union-') as tmp:
                stage = Path(tmp)
                env = dict(os.environ, QT_STYLE_OVERRIDE='Union', QT_QUICK_CONTROLS_STYLE='org.kde.union',
                           UNION_STYLE_NAME='sequoia-popup-prototype', QT_QPA_PLATFORM='offscreen',
                           QT_QPA_PLATFORMTHEME='kde', QT_QUICK_BACKEND='software', QSG_RHI_BACKEND='opengl',
                           LIBGL_ALWAYS_SOFTWARE='1', QT_NO_XDG_DESKTOP_PORTAL='1', PYTHONFAULTHANDLER='1',
                           QT_FORCE_STDERR_LOGGING='1')
                for key in ('QML_IMPORT_PATH', 'QT_PLUGIN_PATH', 'LD_PRELOAD', 'DISPLAY', 'WAYLAND_DISPLAY'):
                    env.pop(key, None)
                for name in ('config', 'data', 'cache', 'state', 'runtime'):
                    path = stage / name; path.mkdir(mode=0o700)
                    env['XDG_' + name.upper() + ('_DIR' if name == 'runtime' else '_HOME')] = str(path)
                scheme = ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}Solid.colors'
                shutil.copy2(scheme, stage / 'config/kdeglobals')
                styles = stage / 'data/union/styles'
                shutil.copytree(STYLE, styles / STYLE.name)
                shutil.copytree('/usr/share/union/styles/breeze', styles / 'breeze')
                config = configparser.ConfigParser(); config.read(scheme)
                def rgb(section, key): return config[f'Colors:{section}'][key]
                bg = rgb('Window', 'BackgroundNormal')
                fg = rgb('Window', 'ForegroundNormal')
                accent = rgb('Selection', 'BackgroundNormal')
                # Same white tint as the shared QML material, with opaque islands.
                amount = 0x12 / 255 if mode == 'dark' else 0x70 / 255
                surface = ','.join(str(round(int(c) * (1-amount) + 255*amount)) for c in bg.split(',')[:3])
                css = f''':root {{
 --sequoia-surface: rgb({surface});
 --sequoia-glass: rgba({bg}, {tokens['glassOpacity']});
 --sequoia-text: rgb({fg});
 --sequoia-accent: rgb({accent});
 --sequoia-rim: rgba({fg}, {tokens['rimOpacity']});
}}\n'''
                (styles / STYLE.name / 'contents/css/palette.css').write_text(css)
                command = ['dbus-run-session', '--', sys.executable, __file__, '--session' if args.native else '--child', kind,
                           '--mode', mode, '--output', str(args.output)]
                result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=50)
                (args.output / f'union-{kind}-{mode}.log').write_text(result.stdout + result.stderr)
                if result.returncode:
                    raise RuntimeError(f'{kind} {mode} exited {result.returncode}\n' + result.stdout + result.stderr)
                print(result.stdout.strip())


if __name__ == '__main__':
    main()
