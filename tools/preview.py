#!/usr/bin/env python3
"""Interactive KSvg preview, or an offscreen PNG with --output FILE."""
import argparse
import os
from pathlib import Path
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--output', type=Path)
args = parser.parse_args()
if args.output:
    os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
os.environ.setdefault('QT_QUICK_BACKEND','software')

from PySide6.QtCore import QTimer, QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView

root = Path(__file__).resolve().parents[1]
app = QGuiApplication(sys.argv)
view = QQuickView()
view.setTitle('Sequoia Eleven — Plasma style preview')
view.rootContext().setContextProperty('projectRoot', str(root))
view.rootContext().setContextProperty('initialDark', args.dark)
view.setSource(QUrl.fromLocalFile(str(root/'tools/preview/Preview.qml')))
if view.status() == QQuickView.Error:
    sys.exit(1)
view.show()

def capture():
    args.output.parent.mkdir(parents=True,exist_ok=True)
    image = view.grabWindow()
    if image.isNull() or not image.save(str(args.output)):
        print('Failed to save preview', file=sys.stderr)
        app.exit(1)
    else:
        print(f'Saved {args.output}')
        app.quit()

if args.output:
    QTimer.singleShot(1500,capture)
sys.exit(app.exec())
