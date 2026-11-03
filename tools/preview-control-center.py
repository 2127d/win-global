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
parser.add_argument('--edit', action='store_true', help='Render the tile editor')
parser.add_argument('--layout', type=Path, help='Preview a saved tile layout JSON')
parser.add_argument('--large-font', action='store_true')
parser.add_argument('--rtl', action='store_true')
parser.add_argument('--width', type=int, default=442)
parser.add_argument('--height', type=int, default=650)
parser.add_argument('--scroll-end', action='store_true', help='Show the bottom of the tile list')
parser.add_argument('--output', type=Path, help='Save the preview at this path')
args = parser.parse_args()
if not args.live:
    from preview_session import ensure_private_session
    ensure_private_session()
ROOT = Path(__file__).resolve().parents[1]
preview_config = tempfile.TemporaryDirectory(prefix='sequoia-control-preview-')
scheme = ROOT / f'themes/color-schemes/SequoiaEleven{"Dark" if args.dark else "Light"}Solid.colors'
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
view.setSource(QUrl.fromLocalFile(str(ROOT / 'desktop/control-center/Preview.qml')))
assert view.status() == QQuickView.Ready, view.errors()
root = view.rootObject()
view.resize(args.width, args.height)
root.setProperty('live', args.live)
if args.layout:
    root.setProperty('savedLayout', args.layout.read_text())
view.show()
if args.edit:
    QTimer.singleShot(500, lambda: QMetaObject.invokeMethod(root.findChild(QObject, 'dashboardViewport'), 'beginEdit'))
if args.scroll_end:
    def scroll_to_end():
        flickable = root.findChild(QObject, 'quickSettingsFlickable')
        flickable.setProperty('contentY', max(0, flickable.property('contentHeight') - flickable.property('height')))
    QTimer.singleShot(1000, scroll_to_end)

def finish():
    try:
        assert not errors, errors
        suffix = ('-live' if args.live else '-dark' if args.dark else '-light')
        if args.large_font: suffix += '-large'
        if args.rtl: suffix += '-rtl'
        output = args.output or ROOT / ('preview/control-center' + suffix + '.png')
        assert view.grabWindow().save(str(output))
        print(output)
        assert not errors, errors
        app.exit(0)
    except Exception as error:
        print(repr(error), file=sys.stderr)
        app.exit(1)
QTimer.singleShot(1800, finish)
sys.exit(app.exec())
