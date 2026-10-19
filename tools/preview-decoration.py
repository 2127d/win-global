#!/usr/bin/env python3
"""Load the compiled decoration in KDE's real decoration preview engine."""
import argparse
import configparser
import os
from pathlib import Path
import sys
import tempfile
from typography import font

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark',action='store_true')
parser.add_argument('--material', choices=['solid', 'transparent'])
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
temporary=tempfile.TemporaryDirectory(prefix='sequoia-decoration-')
config=Path(temporary.name)
(config/'kwinrc').write_text('[org.kde.kdecoration2]\nButtonsOnLeft=XIA\nButtonsOnRight=\nBorderSize=None\n')
(config/'kdeglobals').write_text((root/'themes/color-schemes'/f'SequoiaEleven{"Dark" if args.dark else "Light"}.colors').read_text()
                               + f'\n[General]\nfont={font()}\n[WM]\nactiveFont={font(10, 500)}\n')
os.environ.update(XDG_CONFIG_HOME=str(config),QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='Fusion')
from PySide6.QtCore import QCoreApplication, QUrl, QTimer
from PySide6.QtGui import QColor
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
QCoreApplication.addLibraryPath(str(root/'.staging/desktop/usr/lib64/qt6/plugins'))
app=QApplication(sys.argv)
view=QQuickView()
view.setColor(QColor('transparent'))
view.rootContext().setContextProperty('previewDark',args.dark)
view.rootContext().setContextProperty('previewPlugin', 'org.kde.sequoiaeleven' + ('.' + args.material if args.material else ''))
view.setSource(QUrl.fromLocalFile(str(root/'tools/preview/Decoration.qml')))
assert view.status() == QQuickView.Ready, view.errors()
view.show()
def capture():
    assert view.rootObject().property('pluginLoaded'), 'Decoration plugin failed to load'
    suffix = '-' + args.material if args.material else ''
    output=root/'preview'/f'decoration-{"dark" if args.dark else "light"}{suffix}.png'
    assert view.grabWindow().save(str(output))
    print('Rendered native decoration:',output)
    if args.material:
        view.rootObject().setProperty('alphaProbe', True)
        QTimer.singleShot(100, check_alpha)
    else:
        app.quit()
def check_alpha():
    # Sample the native titlebar away from caption glyphs and window buttons,
    # over an empty client/background so its own alpha remains measurable.
    rendered = view.grabWindow()
    color = rendered.pixelColor(300, 170)
    alpha = color.alpha()
    valid = 0 < alpha < 250 if args.material == 'transparent' else alpha == 255
    scheme = configparser.ConfigParser()
    scheme.read(root/'themes/color-schemes'/f'SequoiaEleven{"Dark" if args.dark else "Light"}{args.material.title()}.colors')
    expected = tuple(map(int, scheme['Colors:Window']['BackgroundNormal'].split(',')))
    if len(expected) == 3:
        expected += (255,)
    valid = valid and all(abs(a-b) <= 1 for a, b in zip(color.getRgb(), expected))
    print(f'Titlebar RGBA: {color.getRgb()}; canvas palette: {expected}')
    print(f'Titlebar alpha: {alpha}/255')
    app.exit(0 if valid else 1)
QTimer.singleShot(1000,capture)
sys.exit(app.exec())
