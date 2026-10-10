#!/usr/bin/env python3
"""Render/test the actual dashboard, optionally reading native device state."""
import argparse
import os
from pathlib import Path
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--live', action='store_true')
parser.add_argument('--large-font', action='store_true')
parser.add_argument('--rtl', action='store_true')
args = parser.parse_args()
if not args.live:
    from preview_session import ensure_private_session
    ensure_private_session()
ROOT = Path(__file__).resolve().parents[1]
preview_config = tempfile.TemporaryDirectory(prefix='sequoia-control-preview-')
scheme = ROOT / f'color-schemes/SequoiaEleven{"Dark" if args.dark else "Light"}Solid.colors'
(Path(preview_config.name) / 'kdeglobals').write_text(scheme.read_text() + f'\n[Icons]\nTheme=SequoiaTahoe-{"dark" if args.dark else "light"}\n')
(Path(preview_config.name) / 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{"dark" if args.dark else "light"}\n')
os.environ['XDG_CONFIG_HOME'] = preview_config.name
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='rhi', QSG_RHI_BACKEND='opengl', QT_QUICK_CONTROLS_STYLE='org.kde.desktop')
from PySide6.QtCore import QUrl, QTimer, QObject, QMetaObject, Qt, qInstallMessageHandler
from PySide6.QtGui import QPalette, QColor, QFont
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

errors = []
def message(kind, context, text):
    print(text, file=sys.stderr)
    if any(s in text for s in ('ReferenceError', 'TypeError', 'Binding loop', 'Unable to assign', 'Cannot assign', 'is not a type')):
        errors.append(text)
qInstallMessageHandler(message)
app = QApplication([])
palette = QPalette()
for role, light, dark in (
    (QPalette.Window, '#edf0f6', '#20232b'), (QPalette.WindowText, '#202530', '#f3f4f7'),
    (QPalette.Base, '#ffffff', '#30343f'), (QPalette.Text, '#202530', '#f3f4f7'),
    (QPalette.Button, '#ffffff', '#30343f'), (QPalette.ButtonText, '#202530', '#f3f4f7'),
    (QPalette.Highlight, '#3676e8', '#3979e9'), (QPalette.HighlightedText, '#ffffff', '#ffffff')):
    palette.setColor(role, QColor(dark if args.dark else light))
palette.setColor(QPalette.Disabled, QPalette.WindowText, QColor('#9ba2b1' if args.dark else '#657085'))
palette.setColor(QPalette.Disabled, QPalette.Text, QColor('#9ba2b1' if args.dark else '#657085'))
app.setPalette(palette)
app.setFont(QFont('Adwaita Sans', 13 if args.large_font else 10))
if args.rtl: app.setLayoutDirection(Qt.RightToLeft)
view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(ROOT / 'plasma/control-center/Preview.qml')))
assert view.status() == QQuickView.Ready, view.errors()
root = view.rootObject()
root.setProperty('live', args.live)
view.show()

def finish():
    try:
        assert not errors, errors
        suffix = ('-live' if args.live else '-dark' if args.dark else '-light')
        if args.large_font: suffix += '-large'
        if args.rtl: suffix += '-rtl'
        output = ROOT / ('preview/control-center' + suffix + '.png')
        assert view.grabWindow().save(str(output))
        print(output)
        if not args.live:
            # Keyboard activation exercises real controls, with only services replaced.
            fixture = root.property('fixture')
            for name, prop in [('Wi-FiToggle', 'wifiEnabled'), ('BluetoothToggle', 'bluetoothEnabled'),
                               ('Do Not DisturbToggle', 'dndEnabled'), ('MicrophoneToggle', 'microphoneMuted'),
                               ('muteButton', 'muted'), ('playPauseButton', 'playing')]:
                button = root.findChild(QObject, name)
                assert button, name
                before = fixture.property(prop)
                button.forceActiveFocus()
                QTest.keyClick(view, Qt.Key_Space)
                assert fixture.property(prop) != before, (name, prop)
            slider = root.findChild(QObject, 'volumeSlider')
            before = fixture.property('volume')
            slider.forceActiveFocus()
            QTest.keyClick(view, Qt.Key_Right)
            assert fixture.property('volume') != before, 'Volume keyboard control did not reach backend'
            fixture.setProperty('volume', 31)
            QTest.qWait(50)
            assert round(slider.property('value')) == 31, 'External volume changes no longer update slider'
            print('PASS: six direct controls, keyboard volume, and external volume synchronization')
        assert not errors, errors
        app.exit(0)
    except Exception as error:
        print(repr(error), file=sys.stderr)
        app.exit(1)
QTimer.singleShot(1800, finish)
sys.exit(app.exec())
