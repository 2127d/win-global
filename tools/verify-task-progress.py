#!/usr/bin/env python3
"""Render task progress and reject disappearing pixels as progress increases."""
import os
import argparse
from pathlib import Path
import tempfile

temporary = tempfile.TemporaryDirectory(prefix='sequoia-task-progress-')
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                  QT_QPA_PLATFORMTHEME='generic',
                  XDG_CONFIG_HOME=temporary.name + '/config',
                  XDG_DATA_HOME=temporary.name + '/data',
                  XDG_CACHE_HOME=temporary.name + '/cache',
                  DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-task-progress')

from PySide6.QtCore import QUrl, Qt, QFile, QPluginLoader, QLibraryInfo
from PySide6.QtGui import QColor
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--stock', action='store_true', help='Test the installed, unmodified KDE progress overlay')
args = parser.parse_args()
app = QApplication([])
source = ROOT / 'desktop/control-center/taskmanager/TaskProgressOverlay.qml'
package = ROOT / 'widgets/plasmoids/org.sequoia.icontasks.animated/contents/ui/TaskProgressOverlay.qml'
assert source.read_text() == package.read_text(), 'packaged progress overlay is stale'
overlay = source.read_text()
if args.stock:
    plugin = Path(QLibraryInfo.path(QLibraryInfo.PluginsPath)) / 'plasma/applets/org.kde.plasma.taskmanager.so'
    loader = QPluginLoader(str(plugin))
    assert loader.load(), loader.errorString()
    resource = QFile(':/qt/qml/plasma/applet/org/kde/plasma/taskmanager/TaskProgressOverlay.qml')
    assert resource.open(QFile.ReadOnly), 'Cannot read the installed KDE progress overlay'
    overlay = bytes(resource.readAll()).decode()
# Supply progress and theme directly, without loading a live panel or app backend.
overlay = overlay.replace('import org.kde.plasma.plasmoid', '')
overlay = overlay.replace('import plasma.applet.org.kde.plasma.taskmanager as TaskManagerApplet', '')
overlay = overlay.replace('value: task.smartLauncherItem.progress', 'value: sampleProgress')
overlay = overlay.replace('id: control', 'id: control\n    width: sampleSize; height: sampleSize\n    property real sampleProgress: 0\n    property real sampleSize: 64\n    property bool reverse: false\n    LayoutMirroring.enabled: reverse')
overlay = overlay.replace('TaskManagerApplet.TaskTools.taskPrefix("progress", Plasmoid.location).concat(TaskManagerApplet.TaskTools.taskPrefix("hover", Plasmoid.location))', '["south-progress", "progress", "south-hover", "hover"]')

view = QQuickView()
view.setColor(QColor('transparent'))
cases = 0
for theme in ('themes/plasma/desktoptheme/sequoia-eleven-light',
              'themes/plasma/desktoptheme/sequoia-eleven-dark',
              'themes/application-style/SequoiaEleven/desktoptheme'):
    qml = Path(temporary.name) / 'progress.qml'
    qml.write_text(overlay.replace('"widgets/tasks"', f'"{ROOT / theme}/widgets/tasks.svg"'))
    view.engine().clearComponentCache()
    view.setSource(QUrl.fromLocalFile(str(qml)))
    assert view.status() == QQuickView.Ready, view.errors()
    view.show()
    root = view.rootObject()
    for size in (32, 48, 64, 96):
        root.setProperty('sampleSize', size)
        for reverse in (False, True):
            app.setLayoutDirection(Qt.RightToLeft if reverse else Qt.LeftToRight)
            root.setProperty('reverse', reverse)
            previous = []
            for progress in range(101):
                root.setProperty('sampleProgress', progress)
                for _ in range(3):
                    app.processEvents()
                raster = view.grabWindow()
                assert not raster.isNull(), 'empty render'
                pixels = [raster.pixelColor(x, y).alpha()
                          for y in range(raster.height()) for x in range(raster.width())]
                assert not previous or all(a <= b for a, b in zip(previous, pixels)), (
                    theme, size, reverse, progress, 'progress erased existing fill')
                if progress == 0:
                    assert not any(pixels), 'zero progress has visible fill'
                if progress == 100:
                    assert any(pixels), 'full progress has no fill'
                # The fill must stay inside the requested progress interval.
                edge = raster.width() * progress / 100
                for x in range(raster.width()):
                    outside = x + 1 < raster.width() - edge if reverse else x > edge
                    if outside:
                        assert all(raster.pixelColor(x, y).alpha() == 0
                                   for y in range(raster.height())), (theme, size, reverse, progress, x)
                previous = pixels
            cases += 1
view.close()
print(f'PASS: {"stock KDE" if args.stock else "animated"}: {cases} progress sweeps (0–100%), three themes, four sizes, both text directions.')
