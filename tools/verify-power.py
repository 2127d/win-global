#!/usr/bin/env python3
"""Exercise and render Power Management without changing real power settings."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
from preview_session import ensure_private_session

ensure_private_session()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--large', action='store_true')
parser.add_argument('--rtl', action='store_true')
parser.add_argument('--render-only', action='store_true')
parser.add_argument('--output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
config = tempfile.TemporaryDirectory(prefix='sequoia-power-preview-')
variant = 'Dark' if args.dark else 'Light'
(Path(config.name) / 'kdeglobals').write_text((root / f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text())
(Path(config.name) / 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant.lower()}\n')
os.environ.update(XDG_CONFIG_HOME=config.name, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
if not args.render_only:
    subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(root / 'desktop/control-center/power/tests'), '-o', '-,txt'], check=True)
from PySide6.QtCore import QUrl, QTimer, Qt
from PySide6.QtGui import QFont, QPalette, QColor
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
app = QApplication([])
palette = QPalette()
for role in (QPalette.Window, QPalette.Base, QPalette.Button):
    palette.setColor(role, QColor('#252629' if args.dark else '#edf0f6'))
for role in (QPalette.WindowText, QPalette.Text, QPalette.ButtonText):
    palette.setColor(role, QColor('#f2f3f5' if args.dark else '#252629'))
    palette.setColor(QPalette.Disabled, role, QColor('#adb0b8' if args.dark else '#686b73'))
palette.setColor(QPalette.Highlight, QColor('#99ce83'))
palette.setColor(QPalette.HighlightedText, QColor('#202530'))
app.setPalette(palette)
app.setFont(QFont('Adwaita Sans', 13 if args.large else 10))
if args.rtl:
    app.setLayoutDirection(Qt.RightToLeft)
view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(root / 'desktop/control-center/power/Preview.qml')))
assert view.status() == QQuickView.Ready, view.errors()
view.resize(442, 550 if args.large else 440)
view.show()
def finish():
    output = args.output or root / f'preview/power-{variant.lower()}.png'
    output.parent.mkdir(parents=True, exist_ok=True)
    assert view.grabWindow().save(str(output))
    print(output)
    app.exit(0)
QTimer.singleShot(700, finish)
raise SystemExit(app.exec())
