#!/usr/bin/env python3
"""Render the actual patched CompactApplet feedback and shared calendar/tray frames."""
import os
from pathlib import Path
import tempfile

import shell_overrides

temporary = tempfile.TemporaryDirectory(prefix='sequoia-launcher-highlight-')
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                  QT_QPA_PLATFORMTHEME='generic', XDG_CACHE_HOME=temporary.name,
                  XDG_CONFIG_HOME=temporary.name + '/config',
                  XDG_DATA_HOME=temporary.name + '/data',
                  XDG_STATE_HOME=temporary.name + '/state',
                  DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-sequoia-highlight')

from PySide6.QtCore import QObject, QUrl
from PySide6.QtGui import QColor, QGuiApplication
from PySide6.QtQuick import QQuickView

ROOT = Path(__file__).resolve().parents[1]
app = QGuiApplication([])
view = QQuickView()
view.setColor(QColor('transparent'))
patched = shell_overrides.patch_launcher((shell_overrides.SYSTEM_SHELL / shell_overrides.COMPACT).read_text())
feedback = patched[patched.index('    KSvg.FrameSvgItem {'):patched.index('    Timer {')]
feedback = feedback.replace('Plasmoid.pluginName', 'root.pluginName').replace('Plasmoid.location', 'root.location')
feedback = feedback.replace('id: expandedItem', 'id: expandedItem; objectName: "feedback"')
cases = []
for mode in ('light', 'dark'):
    path = ROOT / f'plasma/desktoptheme/sequoia-eleven-{mode}/widgets/tabbar.svg'
    # A theme with no launcher-specific prefix must keep stock shell geometry.
    fallback = Path(temporary.name) / f'{mode}-fallback.svg'
    fallback.write_text('\n'.join(line for line in path.read_text().splitlines() if 'sequoia-launcher' not in line))
    for direction in ('north', 'south', 'east', 'west'):
        for size in (22, 34, 58, 70):
            for margin in (3, 6, 12):
                cases.append((mode, direction, path, 'org.kde.plasma.kickoff', size, size, margin, True))
        for plugin in ('org.kde.plasma.kicker', 'org.kde.plasma.kickerdash'):
            cases.append((mode, direction, path, plugin, 58, 70, 6, True))
        for plugin in ('org.kde.plasma.digitalclock', 'org.kde.plasma.systemtray', 'org.sequoia.startmenu'):
            cases.append((mode, direction, path, plugin, 40, 24, 6, False))
        cases.append((mode, direction, fallback, 'org.kde.plasma.kickoff', 58, 58, 6, False))

for mode, direction, path, plugin, width, height, margin, square in cases:
    location = {'north':'TopEdge', 'south':'BottomEdge', 'east':'RightEdge', 'west':'LeftEdge'}[direction]
    block = feedback.replace('"widgets/tabbar"', '"' + str(path) + '"')
    qml = f'''import QtQuick
import org.kde.ksvg as KSvg
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
Item {{ width: 128; height: 128
    property bool isAppletContainer: true
    function getMargins(edge, all) {{ return {margin}; }}
    Item {{ id: root; x: 24; y: 24; width: {width}; height: {height}
        property string pluginName: "{plugin}"
        property int location: PlasmaCore.Types.{location}
        property bool vertical: location === PlasmaCore.Types.LeftEdge || location === PlasmaCore.Types.RightEdge
        property QtObject plasmoidItem: QtObject {{ property bool expanded: true }}
        {block}
        Component.onCompleted: expandedItem.containerMargins = (edge, all) => {margin}
    }}
}}'''
    source = Path(temporary.name) / 'feedback.qml'
    source.write_text(qml)
    # Clear the component cache between cases using a distinct URL.
    view.engine().clearComponentCache()
    view.setSource(QUrl.fromLocalFile(str(source)))
    assert view.status() == QQuickView.Ready, view.errors()
    view.show()
    for _ in range(10): app.processEvents()
    item = view.rootObject().findChild(QObject, 'feedback')
    assert item.property('squareLauncher') == square, (plugin, path)
    raster = view.grabWindow()
    scale = raster.width() / view.width()
    pixels = [(x,y) for y in range(raster.height()) for x in range(raster.width()) if raster.pixelColor(x,y).alpha()]
    bounds = (min(x for x,y in pixels), min(y for x,y in pixels), max(x for x,y in pixels), max(y for x,y in pixels))
    if square:
        side = min(width,height)
        x, y = 24 + (width-side)/2+4, 24 + (height-side)/2+4
        expected = (x,y,x+side-9,y+side-9)
    else:
        horizontal = direction in ('north','south')
        x,y = 24-(0 if horizontal else margin),24-(margin if horizontal else 0)
        expected = (x,y,x+width+(0 if horizontal else 2*margin)-1,y+height+(2*margin if horizontal else 0)-1)
    expected = (round(expected[0]*scale), round(expected[1]*scale), round((expected[2]+1)*scale)-1, round((expected[3]+1)*scale)-1)
    assert all(abs(a-b)<=1 for a,b in zip(bounds,expected)), (mode,direction,plugin,width,height,margin,bounds,expected)
    assert raster.pixelColor(bounds[0],bounds[1]).alpha()==0, 'rounded corner lost'

# These controls use tabbar directly, without CompactApplet's panel margins.
for mode in ('light','dark'):
    for width,height in ((100,24),(24,24)):
        path=ROOT/f'plasma/desktoptheme/sequoia-eleven-{mode}/widgets/tabbar.svg'
        source=Path(temporary.name)/'shared.qml'
        source.write_text(f'''import QtQuick
import org.kde.ksvg as KSvg
KSvg.FrameSvgItem {{ width: {width}; height: {height}; imagePath: "{path}"; prefix: "north-active-tab" }}''')
        view.engine().clearComponentCache()
        view.setSource(QUrl.fromLocalFile(str(source)))
        assert view.status()==QQuickView.Ready,view.errors()
        for _ in range(10): app.processEvents()
        raster=view.grabWindow()
        x=raster.width()//2
        assert raster.pixelColor(x,0).alpha()>0 and raster.pixelColor(x,raster.height()-1).alpha()>0, 'calendar/tray frame shrank'
view.close()
print(f'PASS: {len(cases)} actual launcher/widget feedback cases and 4 calendar/tray frames at {scale:g}×; square launchers, full shared backgrounds, stock fallback.')
