#!/usr/bin/env python3
"""Render the actual widget SVGs over a blurred sample wallpaper in both modes."""
import os
from pathlib import Path

os.environ.update(QT_QPA_PLATFORM='offscreen', QSG_RHI_BACKEND='opengl', QT_QUICK_BACKEND='rhi')
from PySide6.QtCore import QUrl, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView

root = Path(__file__).resolve().parents[1]
app = QGuiApplication([])
view = QQuickView()
view.rootContext().setContextProperty('projectRoot', str(root))
view.setSource(QUrl.fromLocalFile(str(root/'tools/preview/WidgetGlass.qml')))
assert view.status() == QQuickView.Ready, view.errors()
view.show()


def capture():
    output = root/'preview/widget-glass.png'
    assert view.grabWindow().save(str(output))
    print('Rendered light/dark widget glass:', output)
    app.quit()


QTimer.singleShot(1800, capture)
app.exec()
