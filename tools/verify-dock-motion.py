#!/usr/bin/env python3
"""Exercise the shared motion component without launching applications."""
import os
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from pathlib import Path
from PySide6.QtCore import QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine
from PySide6.QtTest import QTest
app = QGuiApplication([])
engine = QQmlEngine()
source = Path(__file__).resolve().parents[1] / 'plasma/control-center/taskmanager/DockMotion.qml'
component = QQmlComponent(engine, QUrl.fromLocalFile(str(source)))
assert not component.errors(), component.errors()
motion = component.createWithInitialProperties(dict(animationsEnabled=True, hovered=False,
    pressed=False, launching=False, launchEnabled=True, pressEnabled=True,
    hoverStyle=2, strength=12, speed=100))
assert motion, component.errors()
def settle():
    QTest.qWait(max(50, motion.property('duration') + 100))
assert motion.property('zoom') == 1
motion.setProperty('hovered', True); settle()
assert abs(motion.property('zoom') - 1.12) < .001
assert motion.property('offset') < 0
motion.setProperty('hoverStyle', 1); settle()
assert motion.property('zoom') == 1 and motion.property('offset') < 0
motion.setProperty('direction', 1); settle()
assert motion.property('offset') > 0
motion.setProperty('pressed', True); settle()
assert abs(motion.property('zoom') - .94) < .001
motion.setProperty('pressEnabled', False); settle()
assert motion.property('zoom') == 1
motion.setProperty('hovered', False)
motion.setProperty('launching', True)
QTest.qWait(max(30, motion.property('duration') // 2))
assert motion.property('bounce') > 0
motion.setProperty('animationsEnabled', False); settle()
assert motion.property('bounce') == 0 and motion.property('offset') == 0 and motion.property('zoom') == 1
motion.setProperty('launching', False)
motion.setProperty('animationsEnabled', True)
motion.setProperty('launchEnabled', False)
motion.setProperty('launching', True); settle()
assert motion.property('bounce') == 0
baseline = motion.property('duration')
motion.setProperty('speed', 200)
assert motion.property('duration') <= baseline / 2 + 1
print('PASS: hover modes, inward direction, press opt-out, launch motion, cancellation, disabled motion and speed')
