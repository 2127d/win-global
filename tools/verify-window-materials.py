#!/usr/bin/env python3
"""Verify dialog tint and native blur requests, including surface recreation."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
if '--probe' not in sys.argv:
    with tempfile.TemporaryDirectory(prefix='sequoia-blur-test-') as temporary:
        library = Path(temporary) / 'blur-probe.so'
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Gui'], text=True))
        subprocess.run(['c++', '-shared', '-fPIC', str(ROOT / 'tools/blur-request-probe.cpp'), '-o', str(library), *flags], check=True)
        for application in ('dialog-test', 'konsole', 'dolphin', 'plasmashell', 'plasmawindowed'):
            subprocess.run([sys.executable, __file__, '--probe', application], check=True,
                           env=dict(os.environ, LD_PRELOAD=str(library)))
    sys.exit(0)

config = tempfile.TemporaryDirectory(prefix='sequoia-window-config-')
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                  QT_QUICK_BACKEND='software', XDG_CONFIG_HOME=config.name)
(Path(config.name) / 'kdeglobals').write_text((ROOT / 'color-schemes/SequoiaElevenLightTransparent.colors').read_text())
from PySide6.QtCore import QCoreApplication, Qt, QUrl
from PySide6.QtGui import QColor, QPalette, QRegion
from PySide6.QtQuick import QQuickWindow
from PySide6.QtQuickWidgets import QQuickWidget
from PySide6.QtWidgets import QApplication, QDialog, QMainWindow, QLabel, QLineEdit, QVBoxLayout, QStyleFactory

QCoreApplication.addLibraryPath(str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
app = QApplication([sys.argv[-1]])
app.setApplicationName(sys.argv[-1])
style = QStyleFactory.create('SequoiaEleven')
assert style
app.setStyle(style)
palette = app.palette()
palette.setColor(QPalette.Window, QColor(242, 243, 245, 184))
palette.setColor(QPalette.Base, QColor(242, 243, 245))
app.setPalette(palette)

def settle():
    for _ in range(5):
        app.processEvents()

if app.applicationName() in ('plasmashell', 'plasmawindowed'):
    # Model Plasma's own rounded blur request, observed through the same probe
    # as style requests. Showing/exposing a panel must not replace this region
    # with KWindowEffects' empty-region (whole-window) sentinel.
    panel = QQuickWindow()
    panel.resize(400, 60)
    panel.setColor(QColor('transparent'))
    region = QRegion(1, 1, 398, 58, QRegion.Ellipse)
    panel.setProperty('testBlurEnabled', True)
    panel.setProperty('testBlurRegion', region)
    panel.setProperty('testBlurRequests', 1)
    for recreate in (False, True):
        if recreate:
            panel.hide()
            panel.destroy()
        panel.show()
        settle()
        assert panel.property('testBlurRequests') == 1, 'Style overwrote Plasma-owned blur'
        assert panel.property('testBlurEnabled') is True
        assert panel.property('testBlurRegion') == region
    panel.setColor(QColor('white'))
    settle()
    assert panel.property('testBlurRequests') == 1, 'Style disabled Plasma-owned blur'
    panel.close()

    fan = QQuickWindow()
    fan.setProperty('sequoiaNoBlur', True)
    fan.setColor(QColor('transparent'))
    fan.resize(400, 300)
    fan.show()
    settle()
    assert fan.property('testBlurEnabled') is False, 'Downloads blur opt-out lost'
    fan.close()
    print(f'PASS ({sys.argv[-1]}): Plasma-owned blur survives show, exposure, surface recreation and color changes; Downloads opt-out retained')
    sys.exit(0)

main = QMainWindow()
main.resize(600, 400)
main.show()
settle()
for parent, kind in ((None, Qt.Dialog), (main, Qt.Dialog), (main, Qt.Tool), (main, Qt.Popup)):
    dialog = QDialog(parent, kind)
    dialog.resize(480, 300)
    layout = QVBoxLayout(dialog)
    layout.addWidget(QLabel('Application settings'))
    layout.addStretch()
    layout.addWidget(QLineEdit('Opaque input'))
    dialog.show()
    settle()
    window = dialog.windowHandle()
    assert dialog.testAttribute(Qt.WA_TranslucentBackground)
    assert dialog.grab().toImage().pixelColor(240, 150).alpha() == 184
    assert window.property('testBlurEnabled') is True, 'Dialog did not request blur'
    assert window.property('testBlurRegion').contains(dialog.rect().center())
    count = window.property('testBlurRequests')
    dialog.hide()
    dialog.show()
    settle()
    assert window.property('testBlurRequests') > count
    assert window.property('testBlurEnabled') is True
    mask = QRegion(10, 10, 400, 240)
    dialog.setMask(mask)
    dialog.resize(490, 310)
    settle()
    assert window.property('testBlurEnabled') is True
    assert (window.property('testBlurRegion') - mask).isEmpty()
    dialog.close()

# No QML style items are present: blur must not depend on drawing a control.
quick = QQuickWindow()
quick.setColor(QColor(242, 243, 245, 184))
quick.resize(400, 300)
quick.show()
settle()
assert quick.property('testBlurEnabled') is True, 'Plain Quick window did not request blur'
quick.hide()
quick.destroy()
count = quick.property('testBlurRequests')
quick.show()
settle()
assert quick.property('testBlurRequests') > count
assert quick.property('testBlurEnabled') is True
quick.setColor(QColor(242, 243, 245))
settle()
assert quick.property('testBlurEnabled') is False, 'Opaque Quick window retained blur'
quick.close()

# An ordinary host, without the old FocusHackWidget class-name special case.
host = QDialog()
host.resize(480, 300)
layout = QVBoxLayout(host)
embedded = QQuickWidget()
embedded.setClearColor(QColor('white'))
layout.addWidget(embedded)
host.show()
settle()
assert embedded.quickWindow().color().alpha() == 0
assert not embedded.testAttribute(Qt.WA_TranslucentBackground), 'Child acquired window translucency'
assert host.grab().toImage().pixelColor(240, 150).alpha() == 184
app.setStyle('Fusion')
settle()
assert embedded.quickWindow().color() == QColor('white'), 'Embedded clear color was not restored'
assert host.windowHandle().property('testBlurEnabled') is False, 'Unpolish retained blur'
host.close()
main.close()
print(f'PASS ({sys.argv[-1]}): dialog tint/blur, masks, reopen, Quick surface recreation, opaque fallback, embedded scene and cleanup')
