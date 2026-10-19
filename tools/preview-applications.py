#!/usr/bin/env python3
"""Render actual Qt widgets using the built Sequoia Eleven style."""
import argparse
import configparser
import os
from pathlib import Path
import sys
import tempfile
from typography import FONT_FAMILY

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--material', choices=['solid', 'transparent'])
args = parser.parse_args()
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
# Isolate preview settings from the active desktop and explicitly load the staged
# plugin, avoiding a second copy of the installed application style.
preview_config = tempfile.TemporaryDirectory(prefix='sequoia-application-preview-')
root = Path(__file__).resolve().parents[1]
scheme_id = f'SequoiaEleven{"Dark" if args.dark else "Light"}{args.material.title() if args.material else ""}'
(Path(preview_config.name)/'kdeglobals').write_text((root/'themes/color-schemes'/f'{scheme_id}.colors').read_text())
os.environ['XDG_CONFIG_HOME'] = preview_config.name
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'
from PySide6.QtCore import QCoreApplication, QTimer, Qt, QSize
from PySide6.QtGui import QColor, QPalette, QFont, QIcon
from PySide6.QtWidgets import (QApplication, QStyleFactory, QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout, QLabel, QListWidget, QTabWidget, QGroupBox,
    QFormLayout, QLineEdit, QComboBox, QCheckBox, QSlider, QProgressBar,
    QPushButton, QTableWidget, QTableWidgetItem, QDockWidget, QToolBar, QFrame)

root = Path(__file__).resolve().parents[1]
QCoreApplication.addLibraryPath(str(root/'.staging/desktop/usr/lib64/qt6/plugins'))
app = QApplication(sys.argv)
style = QStyleFactory.create('SequoiaEleven')
assert style is not None, 'The compiled SequoiaEleven style plugin did not load'
app.setStyle(style)
app.setFont(QFont(FONT_FAMILY, 10))
scheme = configparser.ConfigParser()
scheme.read(root/'themes/color-schemes'/f'{scheme_id}.colors')
palette = QPalette()
for role, section, key in [
    ('Window','Window','BackgroundNormal'), ('WindowText','Window','ForegroundNormal'),
    ('Base','View','BackgroundNormal'), ('AlternateBase','View','BackgroundAlternate'),
    ('Text','View','ForegroundNormal'), ('Button','Button','BackgroundNormal'),
    ('PlaceholderText','View','ForegroundInactive'),
    ('ButtonText','Button','ForegroundNormal'), ('Highlight','Selection','BackgroundNormal'),
    ('HighlightedText','Selection','ForegroundNormal'), ('ToolTipBase','Tooltip','BackgroundNormal'),
    ('ToolTipText','Tooltip','ForegroundNormal')]:
    palette.setColor(getattr(QPalette,role), QColor(*map(int,scheme[f'Colors:{section}'][key].split(','))))
app.setPalette(palette)
window = QMainWindow()
window.setWindowTitle('Sequoia Eleven — Application style')
window.resize(1060, 700)
toolbar = QToolBar('Workspace'); toolbar.setObjectName('workspaceToolbar')
toolbar.setToolButtonStyle(Qt.ToolButtonTextBesideIcon); toolbar.setIconSize(QSize(18,18))
window.addToolBar(toolbar)
for icon, text in [('go-previous','Back'), ('go-next','Forward')]:
    toolbar.addAction(QIcon.fromTheme(icon), text)
toolbar.addSeparator()
toolbar.addAction(QIcon.fromTheme('view-refresh'), 'Refresh')
search = QLineEdit(); search.setPlaceholderText('Search your workspace'); search.setMinimumWidth(280)
toolbar.addWidget(search)
toolbar.addSeparator(); toolbar.addAction(QIcon.fromTheme('configure'), 'Customize')
body = QWidget(); window.setCentralWidget(body)
layout = QHBoxLayout(body); layout.setContentsMargins(18,12,18,18); layout.setSpacing(22)
dock = QDockWidget('Workspace', window); dock.setObjectName('workspaceDock')
sidebar = QListWidget(); sidebar.setMinimumWidth(172); sidebar.setFrameShape(QFrame.NoFrame)
sidebar.setProperty('_kde_side_panel_view', True); sidebar.setSpacing(4)
sidebar.addItems(['Overview', 'Appearance', 'Desktop', 'Notifications', 'Privacy', 'About'])
sidebar.setCurrentRow(1); dock.setWidget(sidebar); window.addDockWidget(Qt.LeftDockWidgetArea, dock)
content = QVBoxLayout(); content.setSpacing(16); layout.addLayout(content)
eyebrow = QLabel('SEQUOIA ELEVEN'); eyebrow.setFont(QFont(FONT_FAMILY,9,QFont.DemiBold)); content.addWidget(eyebrow)
heading = QLabel('Room to float.'); heading.setFont(QFont(FONT_FAMILY,25,QFont.DemiBold)); content.addWidget(heading)
content.addWidget(QLabel('Floating toolbars. Quiet surfaces. A workspace with room to breathe.'))
tabs = QTabWidget(); content.addWidget(tabs)
page = QWidget(); tabs.addTab(page,'Personalization'); tabs.addTab(QWidget(),'Workspace'); tabs.addTab(QWidget(),'Advanced')
page_layout = QVBoxLayout(page); page_layout.setContentsMargins(16,18,16,16); page_layout.setSpacing(18)
group = QGroupBox('Your workspace'); form = QFormLayout(group)
entry = QLineEdit(); entry.setPlaceholderText('Name your workspace'); form.addRow('Name',entry)
combo = QComboBox(); combo.addItems(['Sequoia Eleven Dark','Sequoia Eleven Light']);combo.setCurrentIndex(0 if args.dark else 1);form.addRow('Appearance',combo)
check = QCheckBox('Use translucent surfaces');check.setChecked(True);form.addRow('',check)
slider = QSlider(Qt.Horizontal);slider.setValue(64);form.addRow('Transparency',slider);page_layout.addWidget(group)
table = QTableWidget(3,3); table.setMinimumHeight(150); table.setHorizontalHeaderLabels(['Surface','Finish','Status']);table.verticalHeader().hide()
for row, values in enumerate([['Panels','Charcoal glass' if args.dark else 'Porcelain glass','Ready'],['Windows','Rounded · centered titles','Ready'],['Applications','Sequoia Eleven / Qt 6','Ready']]):
    for col,value in enumerate(values):table.setItem(row,col,QTableWidgetItem(value))
table.horizontalHeader().setStretchLastSection(True);table.setColumnWidth(0,135);table.setColumnWidth(1,275);page_layout.addWidget(table)
progress=QProgressBar();progress.setValue(72);progress.setTextVisible(False);page_layout.addWidget(progress)
actions=QHBoxLayout();actions.addStretch();actions.addWidget(QPushButton('Cancel'))
save=QPushButton('Save changes');save.setDefault(True);actions.addWidget(save);content.addLayout(actions)
window.show()
toolbar.setPalette(palette)
sidebar.setFocus()
def capture():
    suffix = '-' + args.material if args.material else ''
    path=root/'preview'/f'applications-{"dark" if args.dark else "light"}{suffix}.png'
    rendered = window.grab().toImage()
    if args.material:
        samples = {'canvas': (window.width()-8, window.height()//2),
                   'toolbar': (toolbar.width()-30, toolbar.height()//2),
                   'sidebar': (dock.width()//2, dock.y()+dock.height()-40)}
        from PySide6.QtCore import QPoint
        content_point = table.viewport().mapTo(window, QPoint(table.viewport().width()-20, table.viewport().height()-15))
        samples['content'] = (content_point.x(), content_point.y())
        for name, widget in [('group', group), ('tab page', page)]:
            point = widget.mapTo(window, QPoint(widget.width()-20, widget.height()-20))
            samples[name] = (point.x(), point.y())
        for name, (x, y) in samples.items():
            scale = rendered.devicePixelRatio()
            alpha = rendered.pixelColor(round(x * scale), round(y * scale)).alpha()
            glass = args.material == 'transparent' and name == 'canvas'
            assert (0 < alpha < 250) if glass else alpha == 255, (name, alpha)
            print(f'{name} alpha: {alpha}/255')
    assert rendered.save(str(path))
    print('Rendered actual SequoiaEleven Qt style:', path)
    app.quit()
QTimer.singleShot(800,capture)
sys.exit(app.exec())
