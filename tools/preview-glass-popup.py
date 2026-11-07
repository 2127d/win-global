#!/usr/bin/env python3
"""Fixture for the private compositor; never launched by the live trial."""
from pathlib import Path
from PySide6.QtCore import QUrl
from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlApplicationEngine
app=QApplication([]);engine=QQmlApplicationEngine()
engine.load(QUrl.fromLocalFile(str(Path(__file__).with_name('preview')/'GlassPopup.qml')))
if not engine.rootObjects():raise RuntimeError('Glass popup fixture failed to load')
app.exec()
