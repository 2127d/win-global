#!/usr/bin/env python3
"""Verify ordinary Qt controls inherit Sequoia without application makeovers."""
import argparse
import json
import os
from pathlib import Path
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--plugins', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--glass', action='store_true')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--quick', action='store_true')
parser.add_argument('--quick-runner', type=Path, help='Compiled verify-standard-quick.cpp harness')
parser.add_argument('--baseline', action='store_true')
parser.add_argument('--show', action='store_true')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
state = tempfile.TemporaryDirectory(prefix='sequoia-standard-controls-')
os.environ.update(XDG_CONFIG_HOME=state.name, XDG_DATA_HOME=state.name,
                  QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='SequoiaElevenGlass' if args.glass else 'SequoiaEleven',
                  QT_PLUGIN_PATH=str(args.plugins.resolve()))
if not args.show:
    os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
    os.environ.setdefault('QT_QUICK_BACKEND', 'software')
# Deliberately no component imports, makeover launcher, or app-specific overlay.
for key in ('QML_IMPORT_PATH', 'QML2_IMPORT_PATH', 'QT_QUICK_CONTROLS_STYLE'):
    os.environ.pop(key, None)
(Path(state.name) / 'sequoiaelevenrc').write_text('[Common]\nSunkenEffect=false\n[Style]\nAnimationsEnabled=false\n[Windeco]\nGlassRimIntensity=100\n')
if args.quick:
    import subprocess
    if not args.quick_runner:
        parser.error('--quick requires --quick-runner')
    command = [str(args.quick_runner.resolve()), '--output', str(args.output.resolve()),
               '--qml', str(Path(__file__).with_name('standard-controls.qml'))]
    for name in ('dark', 'glass', 'baseline', 'show'):
        if getattr(args, name): command.append('--' + name)
    raise SystemExit(subprocess.call(command))
from PySide6.QtCore import Qt, QRect
from PySide6.QtGui import QColor, QImage, QPainter, QPalette
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QFormLayout,
    QGroupBox, QPushButton, QLineEdit, QComboBox, QSpinBox, QSlider, QCheckBox,
    QRadioButton, QProgressBar, QToolBar, QLabel, QStyle, QStyleOptionButton, QStyleOptionFrame)
from PySide6.QtTest import QTest
app = QApplication([])
style = app.style()
assert style.objectName().lower() == ('sequoiaelevenglass' if args.glass else 'sequoiaeleven'), style.objectName()
if not args.baseline:
    assert style.property('sequoiaSharedControls'), 'Updated plugin was not loaded'
palette = app.palette()
for role, color in [(QPalette.Window, '#252629' if args.dark else '#f2f3f5'),
                    (QPalette.Base, '#242528' if args.dark else '#ffffff'),
                    (QPalette.Button, '#303135' if args.dark else '#ffffff'),
                    (QPalette.WindowText, '#f2f3f5' if args.dark else '#24262b'),
                    (QPalette.Text, '#f2f3f5' if args.dark else '#24262b'),
                    (QPalette.ButtonText, '#f2f3f5' if args.dark else '#24262b'),
                    (QPalette.Highlight, '#74a7e7'), (QPalette.HighlightedText, '#101820')]:
    palette.setColor(role, QColor(color))
app.setPalette(palette)
stem = ('quick' if args.quick else 'widgets') + ('-dark' if args.dark else '-light') + ('-glass' if args.glass else '-solid')
report = {'style': style.objectName(), 'app_specific_overlays': False, 'platform': app.platformName()}
window = QMainWindow(); window.resize(820, 620)
window.setWindowTitle('Sequoia · ordinary Qt controls')
toolbar = QToolBar('Standard toolbar'); window.addToolBar(toolbar)
toolbar.addAction('Back'); toolbar.addAction('Forward'); toolbar.addSeparator(); toolbar.addAction('Refresh')
page = QWidget(); window.setCentralWidget(page); layout = QVBoxLayout(page)
layout.setContentsMargins(26, 24, 26, 24); layout.setSpacing(18)
title = QLabel('Ordinary apps. Shared Sequoia controls.'); font = title.font(); font.setPointSize(18); title.setFont(font)
layout.addWidget(title); layout.addWidget(QLabel('These are standard Qt widgets, with no component imports or custom stylesheet.'))
group = QGroupBox('Controls supplied by the application style'); layout.addWidget(group)
box = QVBoxLayout(group); box.setSpacing(18)
row = QHBoxLayout(); box.addLayout(row)
action = QPushButton('Ordinary button'); checked = QPushButton('Checked'); checked.setCheckable(True); checked.setChecked(True)
disabled = QPushButton('Disabled'); disabled.setEnabled(False)
for b in (action, checked, disabled): row.addWidget(b)
triggered = []; action.clicked.connect(lambda: triggered.append(1)); disabled.clicked.connect(lambda: triggered.append(2))
form = QFormLayout(); box.addLayout(form)
field = QLineEdit(); field.setPlaceholderText('Type in a standard text field'); form.addRow('Text field', field)
combo = QComboBox(); combo.addItems(['First option', 'Second option', 'Third option']); form.addRow('Choice', combo)
spin = QSpinBox(); spin.setValue(24); form.addRow('Number', spin)
row = QHBoxLayout(); box.addLayout(row)
check = QCheckBox('Tri-state checkbox'); check.setTristate(True); row.addWidget(check)
radio = QRadioButton('First radio'); radio.setChecked(True); row.addWidget(radio)
radio2 = QRadioButton('Second radio'); row.addWidget(radio2)
slider = QSlider(Qt.Horizontal); slider.setValue(60); form.addRow('Slider', slider)
progress = QProgressBar(); progress.setValue(60); box.addWidget(progress)
layout.addStretch()
layout.addWidget(QLabel('The application keeps its layout and input behavior. Sequoia supplies the control surfaces.'))
window.show(); QTest.qWait(100)
action.setFocus(); QTest.keyClick(action, Qt.Key_Space)
QTest.mouseClick(disabled, Qt.LeftButton); assert triggered == [1]
field.setFocus(); QTest.keyClicks(field, 'Native input'); assert field.text() == 'Native input'
spin.setFocus(); QTest.keyClick(spin, Qt.Key_Up); assert spin.value() == 25
combo.setFocus(); QTest.keyClick(combo, Qt.Key_Down); assert combo.currentIndex() == 1
slider.setFocus(); QTest.keyClick(slider, Qt.Key_Right); assert slider.value() == 61
states = []
for _ in range(3): QTest.keyClick(check, Qt.Key_Space); states.append(check.checkState())
assert set(states) == {Qt.Unchecked, Qt.PartiallyChecked, Qt.Checked}
radio2.setFocus(); QTest.keyClick(radio2, Qt.Key_Space)
assert radio2.isChecked() and not radio.isChecked()
report.update(keyboard=True, disabled=True, text_input=True, combo=True, spin=True, slider=True, tristate=True, radio=True)
# Render actual style primitives without a parent fill to inspect their alpha and rounded corners.
for name, option, primitive in [('button', QStyleOptionButton(), QStyle.PE_PanelButtonCommand),
                                ('field', QStyleOptionFrame(), QStyle.PE_FrameLineEdit)]:
    option.rect = QRect(0, 0, 180, 38); option.palette = palette
    option.state = QStyle.State_Enabled | QStyle.State_Active
    image = QImage(180, 38, QImage.Format_ARGB32_Premultiplied); image.fill(Qt.transparent)
    painter = QPainter(image); style.drawPrimitive(primitive, option, painter); painter.end()
    alpha = image.pixelColor(90, 19).alpha()
    if not args.baseline:
        expected = 112 if name == 'button' else 128
        assert abs(alpha - expected) <= 1 if args.glass else alpha == 255, (name, alpha)
        assert image.pixelColor(4, 4).alpha() < 10, 'Control lost its pill corners'
    report[name + '_center_alpha'] = alpha
    image.save(str(args.output / (stem + '-' + name + '.png')))
QTest.qWait(80)
window.grab().save(str(args.output / (stem + '.png')))
for direction in (Qt.LeftToRight, Qt.RightToLeft):
    window.setLayoutDirection(direction); window.resize(680, 620); app.processEvents()
    assert field.isVisible() and field.width() > 100 and not window.grab().isNull()
report['rtl'] = True
(args.output / (stem + '.json')).write_text(json.dumps(report, indent=2)+'\n')
print('PASS', json.dumps(report), flush=True)
if args.show:
    app.exec()
else:
    window.close()
