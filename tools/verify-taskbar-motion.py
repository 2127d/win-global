#!/usr/bin/env python3
"""Verify the shared Sequoia motion as rendered by the taskbar icon wrapper."""
import argparse
import os
from pathlib import Path
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--reduced-motion', action='store_true')
args = parser.parse_args()
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
config = tempfile.TemporaryDirectory(prefix='sequoia-motion-')
os.environ['XDG_CONFIG_HOME'] = config.name
if args.reduced_motion:
    (Path(config.name) / 'kdeglobals').write_text('[KDE]\nAnimationDurationFactor=0\n')

from PySide6.QtCore import QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine
from PySide6.QtTest import QTest

app = QGuiApplication([])
engine = QQmlEngine()
engine.setProperty('_kirigamiTheme', 'org.kde.desktop')
engine.addImportPath(str(Path(__file__).resolve().parents[1] / '.staging/taskbar-build/qml'))
component = QQmlComponent(engine)
component.setData(b'import QtQuick\nimport org.sequoia.taskbar 1.0\nMotionIcon {}', QUrl())
assert not component.errors(), component.errors()
settings = dict(dockAnimations=True, taskHoverEffect=True, dockHoverStyle=2,
    dockStrength=12, dockSpeed=100, dockLaunchAnimation=True,
    dockPressAnimation=True, dockAttentionAnimation=True)
icon = component.createWithInitialProperties(dict(settings=settings, hovered=False,
    pressed=False, viewportWidth=56, viewportHeight=64, width=32, height=32,
    source='system-run'))
assert icon, component.errors()

def settle():
    QTest.qWait(max(50, icon.property('motionDuration') + 100))

def set_settings(**values):
    settings.update(values)
    icon.setProperty('settings', settings)

def check_bounds():
    scale = icon.property('scale')
    offset = abs(icon.property('motionOffset'))
    width, height = icon.property('width') * scale, icon.property('height') * scale
    cross = height if icon.property('horizontal') else width
    extent = icon.property('viewportHeight' if icon.property('horizontal') else 'viewportWidth')
    assert cross + 2 * offset <= extent + .001
    assert width <= icon.property('viewportWidth') + .001
    assert height <= icon.property('viewportHeight') + .001

icon.setProperty('hovered', True)
settle()
if args.reduced_motion:
    assert not icon.property('moving') and icon.property('motionDuration') == 0
    icon.setProperty('launching', True)
    icon.setProperty('pressed', True)
    icon.setProperty('demandingAttention', True)
    settle()
    assert icon.property('scale') == 1 and icon.property('motionOffset') == 0
    assert icon.property('attentionPulse') == 1
    print('PASS: desktop reduced-motion setting disables all taskbar icon effects')
    raise SystemExit(0)

assert abs(icon.property('scale') - 1.12) < .001
assert icon.property('motionOffset') < 0
assert icon.property('width') == 32 and icon.property('height') == 32
for horizontal in (True, False):
    icon.setProperty('horizontal', horizontal)
    for direction in (-1, 1):
        icon.setProperty('inwardDirection', direction)
        settle()
        assert icon.property('motionOffset') * direction > 0
        check_bounds()
icon.setProperty('pressed', True)
settle()
assert abs(icon.property('scale') - .94) < .001
icon.setProperty('pressed', False)
# Extreme settings must not move visual content beyond the button hit area.
set_settings(dockStrength=20)
icon.setProperty('viewportWidth', 32)
icon.setProperty('viewportHeight', 32)
icon.setProperty('launching', True)
for _ in range(30):
    QTest.qWait(20)
    check_bounds()
assert icon.property('scale') == 1 and icon.property('motionOffset') == 0
icon.setProperty('demandingAttention', True)
QTest.qWait(180)
assert icon.property('attentionPulse') < .9
QTest.qWait(400)
assert icon.property('attentionPulse') == 1, 'attention cue must finish while attention remains set'
icon.setProperty('demandingAttention', False)
icon.setProperty('demandingAttention', True)
QTest.qWait(100)
set_settings(dockAnimations=False)
settle()
assert icon.property('attentionPulse') == 1
assert icon.property('scale') == 1 and icon.property('motionOffset') == 0
print('PASS: shared hover/press motion, both axes and directions, fixed geometry, clipping bounds, finite attention pulse and cancellation')
