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
parser.add_argument('--live-switch',action='store_true',help='Verify live glass recoloring without recreating the decoration')
parser.add_argument('--material', choices=['solid', 'transparent', 'glass'])
parser.add_argument('--radius', type=int, choices=range(-1,33), metavar='-1..32')
parser.add_argument('--plugin', help='Decoration plugin override for isolated verification')
parser.add_argument('--plugin-path', type=Path, help='Private Qt plugin root')
parser.add_argument('--output', type=Path, help='Capture path for an isolated comparison')
parser.add_argument('--decoration-size', type=int, choices=range(80,141))
parser.add_argument('--rim-intensity', type=int, choices=range(0,201))
parser.add_argument('--inactive', action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
temporary=tempfile.TemporaryDirectory(prefix='sequoia-decoration-')
config=Path(temporary.name)
if args.radius is not None:
    (config/'sequoiaelevenrc').write_text(f'[Windeco]\nOtherCornerRadius={args.radius}\n')
if args.decoration_size is not None:
    with (config/'sequoiaelevenrc').open('a') as file:
        file.write(f'\n[Windeco]\nDecorationScale={args.decoration_size}\n')
if args.rim_intensity is not None:
    with (config/'sequoiaelevenrc').open('a') as file:
        file.write(f'\n[Windeco]\nGlassRimIntensity={args.rim_intensity}\n')
(config/'kwinrc').write_text('[org.kde.kdecoration2]\nButtonsOnLeft=XIA\nButtonsOnRight=\nBorderSize=None\n')
(config/'kdeglobals').write_text((root/'themes/color-schemes'/f'SequoiaEleven{"Dark" if args.dark else "Light"}.colors').read_text()
                               + f'\n[General]\nfont={font()}\n[WM]\nactiveFont={font(10, 500)}\n')
if args.material == 'glass':
    (config/'kdeglobals').write_text((root/'themes/color-schemes'/f'SequoiaEleven{"Dark" if args.dark else "Light"}Transparent.colors').read_text())
    from app_glass import apply_settings,edits
    apply_settings(config,edits(config,surfaces=True))
os.environ.update(XDG_CONFIG_HOME=str(config),QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='Fusion')
from PySide6.QtCore import QCoreApplication, QUrl, QTimer
from PySide6.QtGui import QColor
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
QCoreApplication.addLibraryPath(str(root/'.staging/desktop/usr/lib64/qt6/plugins'))
if args.plugin_path:QCoreApplication.addLibraryPath(str(args.plugin_path.resolve()))
app=QApplication(sys.argv)
view=QQuickView()
view.setColor(QColor('transparent'))
view.rootContext().setContextProperty('previewDark',args.dark)
view.rootContext().setContextProperty('previewActive',not args.inactive)
view.rootContext().setContextProperty('previewPlugin', args.plugin or ('org.kde.sequoiaeleven' + ('.glass-adjustable' if args.material=='glass' else '.' + args.material if args.material else '')))
view.setSource(QUrl.fromLocalFile(str(root/'tools/preview/Decoration.qml')))
assert view.status() == QQuickView.Ready, view.errors()
view.show()
def capture():
    assert view.rootObject().property('pluginLoaded'), 'Decoration plugin failed to load'
    suffix = ('-' + args.material if args.material else '') + (f'-radius-{args.radius}' if args.radius is not None else '')
    output=args.output or root/'preview'/f'decoration-{"dark" if args.dark else "light"}{suffix}.png'
    output.parent.mkdir(parents=True,exist_ok=True)
    assert view.grabWindow().save(str(output))
    print('Rendered native decoration:',output)
    if args.material:
        view.rootObject().setProperty('alphaProbe', True)
        QTimer.singleShot(100, check_alpha)
    else:
        app.quit()
def check_alpha():
    global switch_stage
    # Sample the native titlebar away from caption glyphs and window buttons,
    # over an empty client/background so its own alpha remains measurable.
    rendered = view.grabWindow()
    scale=rendered.width()/view.width()
    color = rendered.pixelColor(round(300*scale), round(170*scale))
    if args.output:rendered.save(str(args.output.with_stem(args.output.stem+'-alpha')))
    alpha = color.alpha()
    valid = 0 < alpha < 250 if args.material in ('transparent','glass') else alpha == 255
    scheme = configparser.ConfigParser()
    scheme.read(root/'themes/color-schemes'/f'SequoiaEleven{"Dark" if args.dark else "Light"}{('Transparent' if args.material=='glass' else args.material.title())}.colors')
    expected = tuple(map(int, scheme['Colors:Window']['BackgroundNormal'].split(',')))
    if args.material=='glass':expected=expected[:3]+((168 if args.dark else 148),)
    if len(expected) == 3:
        expected += (255,)
    valid = valid and all(abs(a-b) <= 1 for a, b in zip(color.getRgb(), expected))
    print(f'Titlebar RGBA: {color.getRgb()}; canvas palette: {expected}')
    print(f'Titlebar alpha: {alpha}/255')
    if valid and args.live_switch:
        assert args.material=='glass'
        if switch_stage < 2:
            import subprocess
            target_dark = not args.dark if switch_stage == 0 else args.dark
            channels = '31,34,39,168' if target_dark else '242,243,245,148'
            subprocess.run(['kwriteconfig6','--notify','--file','kdeglobals','--group','Colors:Window','--key','BackgroundNormal',channels],check=True)
            switch_stage += 1
            QTimer.singleShot(700, check_switched)
            return
    app.exit(0 if valid else 1)
def check_switched():
    global switch_stage
    rendered=view.grabWindow();scale=rendered.width()/view.width()
    actual=rendered.pixelColor(round(300*scale),round(170*scale)).getRgb()
    target_dark = not args.dark if switch_stage==1 else args.dark
    expected=(31,34,39,168) if target_dark else (242,243,245,148)
    valid=all(abs(a-b)<=1 for a,b in zip(actual,expected))
    print('Live titlebar RGBA:', actual, 'expected:', expected, flush=True)
    if not valid:app.exit(1);return
    if switch_stage==1:
        import subprocess
        channels='31,34,39,168' if args.dark else '242,243,245,148'
        subprocess.run(['kwriteconfig6','--notify','--file','kdeglobals','--group','Colors:Window','--key','BackgroundNormal',channels],check=True)
        switch_stage=2;QTimer.singleShot(700,check_switched)
    else:
        print('PASS: live light/dark decoration palette updates without recreation',flush=True);app.quit()
switch_stage=0
QTimer.singleShot(1000,capture)
sys.exit(app.exec())
