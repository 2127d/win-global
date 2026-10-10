#!/usr/bin/env python3
"""Exercise island layout and interactions against the staged Qt style."""
import os
from pathlib import Path
import tempfile

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
config = tempfile.TemporaryDirectory(prefix='sequoia-island-check-')
os.environ['XDG_CONFIG_HOME'] = config.name

from PySide6.QtCore import QCoreApplication, QMargins, Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import (
    QApplication, QDockWidget, QFrame, QLineEdit, QListWidget, QMainWindow,
    QStyleFactory, QToolBar, QToolButton,
    QWidget, QStackedWidget, QTabWidget, QLabel,
)

root = Path(__file__).resolve().parents[1]
QCoreApplication.addLibraryPath(str(root / '.staging/desktop/usr/lib64/qt6/plugins'))
app = QApplication([])
style = QStyleFactory.create('SequoiaEleven')
assert style, 'Build and stage the application style first'
app.setStyle(style)
window = QMainWindow()
window.resize(900, 600)
view = QListWidget()
view.addItems([f'Item {i}' for i in range(100)])
window.setCentralWidget(view)
dock = QDockWidget('Places', window)
original_margins = QMargins(3, 4, 5, 6)
dock.setContentsMargins(original_margins)
places = QListWidget()
places.setFrameShape(QFrame.NoFrame)
places.setProperty('_kde_side_panel_view', True)
places.addItems(['Home', 'Documents', 'Downloads'])
dock.setWidget(places)
window.addDockWidget(Qt.LeftDockWidgetArea, dock)
locked_dock = QDockWidget('Locked sidebar', window)
locked_content = QStackedWidget()
locked_tabs = QTabWidget()
for label in ('Thumbnails', 'Reviews', 'Bookmarks', 'Contents'):
    locked_tabs.addTab(QLabel(label), label)
locked_tabs.setCurrentIndex(3)
locked_content.addWidget(locked_tabs)
locked_dock.setWidget(locked_content)
locked_dock.setTitleBarWidget(QWidget())
window.addDockWidget(Qt.LeftDockWidgetArea, locked_dock)
toolbar = QToolBar('Navigation')
window.addToolBar(toolbar)
triggered = []
action = toolbar.addAction('Refresh')
action.triggered.connect(lambda: triggered.append(True))
toolbar.addWidget(QLineEdit('Search'))
for i in range(12):
    toolbar.addAction(f'Action {i}')
vertical = QToolBar('Tools')
window.addToolBar(Qt.RightToolBarArea, vertical)
vertical.addAction('One')
vertical.addAction('Two')
stacked = QToolBar('Secondary')
window.addToolBarBreak()
window.addToolBar(stacked)
stacked.addAction('Secondary action')
standalone = QListWidget()
standalone.setFrameShape(QFrame.NoFrame)
standalone.setProperty('_kde_side_panel_view', True)
standalone.addItems(['General', 'Appearance'])
standalone.resize(220, 220)
standalone.show()
window.show()
app.processEvents()
app.processEvents()

assert locked_tabs.mapTo(locked_dock, locked_tabs.rect().topLeft()).y() >= 12, 'Titleless dock tabs escape the island'

assert view.viewport().geometry().left() >= 12
assert view.viewport().geometry().top() >= 12
assert dock.contentsMargins() == QMargins(12, 12, 12, 12)
assert standalone.viewport().geometry().left() >= 12
button = toolbar.widgetForAction(action)
assert button.geometry().left() >= 5 and button.geometry().top() >= 5
QTest.mouseClick(button, Qt.LeftButton)
assert triggered == [True], 'Toolbar action stopped receiving clicks'
view.setCurrentRow(99)
view.scrollToItem(view.item(99))
app.processEvents()
assert view.visualItemRect(view.item(99)).intersects(view.viewport().rect())

for direction in (Qt.LeftToRight, Qt.RightToLeft):
    window.setLayoutDirection(direction)
    for width in (900, 480):
        window.resize(width, 600)
        app.processEvents()
        app.processEvents()
        assert locked_tabs.mapTo(locked_dock, locked_tabs.rect().topLeft()).y() >= 12
        assert not window.grab().isNull()
        assert vertical.widgetForAction(vertical.actions()[0]).geometry().left() >= 5
extension = toolbar.findChild(QToolButton, 'qt_toolbar_ext_button')
assert extension and extension.isVisible(), 'Narrow toolbar must expose overflow actions'
dock.setFloating(True)
app.processEvents()
assert dock.isFloating() and not dock.grab().isNull()
dock.setFloating(False)
app.processEvents()
assert not dock.isFloating()
locked_dock.setFloating(True)
app.processEvents()
app.processEvents()
assert locked_content.contentsMargins().top() == 0, 'Floating dock retained extra inset'
locked_dock.setFloating(False)
app.processEvents()
app.processEvents()
assert locked_tabs.mapTo(locked_dock, locked_tabs.rect().topLeft()).y() >= 12

app.setStyle('Fusion')
app.processEvents()
assert dock.contentsMargins() == original_margins, 'Dock margins were not restored'
assert standalone.frameShape() == QFrame.NoFrame, 'Sidebar frame was not restored'
assert locked_content.contentsMargins() == QMargins(), 'Locked sidebar content margins were not restored'
window.close()
standalone.close()
print('PASS: insets, titleless sidebar tabs, toolbar clicks/overflow, scrolling, RTL, vertical/stacked bars, dock transitions, style cleanup')
