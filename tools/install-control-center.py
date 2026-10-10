#!/usr/bin/env python3
"""Build/install a version-checked dashboard for the existing Plasma system tray."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
from control_center_details import style_details
from control_center_weather import style_weather
from control_center_contacts import style_contacts
from control_center_dock import style_dock
from control_center_notifications import style_notifications

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'plasma/control-center'
PLUGIN = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip()) / 'plasma/applets/org.kde.plasma.systemtray.so'
MEDIA_PLUGIN = PLUGIN.with_name('org.kde.plasma.mediacontroller.so')
MARKER = '.sequoia-control-center'


def replace(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f'Plasma tray changed; review the integration anchor: {old!r}')
    return text.replace(old, new, 1)


def build(destination):
    from PySide6.QtCore import QCoreApplication, QDir, QDirIterator, QFile, QPluginLoader, QLibraryInfo
    app = QCoreApplication.instance() or QCoreApplication([])
    # Keep plugin loaders alive while their resources are being copied.
    loaders = []
    def extract(plugin_path, resource, target_dir):
        loader = QPluginLoader(str(plugin_path))
        if not loader.load():
            raise RuntimeError(loader.errorString())
        loaders.append(loader)
        files = QDirIterator(resource, QDir.Files, QDirIterator.Subdirectories)
        while files.hasNext():
            name = files.next()
            source = QFile(name)
            if not source.open(QFile.ReadOnly):
                raise RuntimeError(f'Cannot read {name}')
            target = target_dir / name.removeprefix(resource)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(bytes(source.readAll()))
        (target_dir / 'qmldir').unlink()
        if not (target_dir / 'main.qml').exists():
            raise RuntimeError(f'No applet entry point: {plugin_path}')
    plugin = QPluginLoader(str(PLUGIN))
    if not plugin.load():
        raise RuntimeError(plugin.errorString())
    tray = destination / 'tray'
    tray.mkdir(parents=True)
    resource = ':/qt/qml/plasma/applet/org/kde/plasma/systemtray/'
    files = QDirIterator(resource, QDir.Files, QDirIterator.Subdirectories)
    while files.hasNext():
        name = files.next()
        source = QFile(name)
        assert source.open(QFile.ReadOnly)
        target = tray / name.removeprefix(resource)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(bytes(source.readAll()))
    # Local sibling QML must not resolve back into the compiled upstream module.
    (tray / 'qmldir').unlink()
    for source in SOURCE.glob('*.qml'):
        if source.name == 'Preview.qml':
            continue
        shutil.copy2(source, tray / source.name)

    path = tray / 'main.xml'
    text = replace(path.read_text(), '  </group>', '''    <entry name="controlCenterWidth" type="Int">
      <label>Default width of the Control Center content in logical pixels.</label>
      <default>297</default><min>260</min><max>1600</max>
    </entry>
    <entry name="controlCenterHeight" type="Int">
      <label>Default height of the Control Center content in logical pixels.</label>
      <default>451</default><min>240</min><max>1600</max>
    </entry>
  </group>''')
    path.write_text(text)
    path = tray / 'config.qml'
    text = replace(path.read_text(), 'ConfigModel {', '''ConfigModel {
    ConfigCategory {
        name: i18n("Size")
        icon: "transform-scale"
        source: "ConfigSize.qml"
    }''')
    path.write_text(text)

    path = tray / 'ExpandedRepresentation.qml'
    text = path.read_text()
    text = replace(text, '    id: popup', '''    id: popup
    objectName: "sequoiaControlCenter"
    property bool showAllItems: false
    ControlBackend { id: controls }
    ControlItems {
        id: trayItems
        objectName: "controlCenterItems"
        configuration: Plasmoid.configuration
        sourceModel: Plasmoid.systemTrayModel
    }
    function goHome() {
        systemTrayState.setActiveApplet(null);
        showAllItems = false;
    }
    function openApplet(plugin) {
        const model = Plasmoid.systemTrayModel;
        const role = model.KItemModels.KRoleNames.role("applet");
        for (let row = 0; row < model.rowCount(); ++row) {
            const applet = model.data(model.index(row, 0), role);
            if (applet && applet.Plasmoid.pluginName === plugin) {
                systemTrayState.setActiveApplet(applet, row);
                return;
            }
        }
        goHome();
    }
    Connections {
        target: systemTrayState
        function onExpandedChanged() {
            if (!systemTrayState.expanded) popup.showAllItems = false;
        }
    }''')
    text = replace(text, 'import QtQuick.Layouts', 'import QtQuick.Layouts\nimport QtQuick.Window\nimport org.kde.kitemmodels as KItemModels')
    # Plasma adds its frame around the configured content size; preserve the
    # original 297x451 default and limit only the dashboard to the saved size.
    # Keep native detail pages wide enough and restore the compact size on Back.
    text = replace(text, '    Layout.minimumWidth: Kirigami.Units.gridUnit * 24', '''    readonly property bool compactDashboard: !systemTrayState.activeApplet && !showAllItems
    readonly property bool weatherPage: systemTrayState.activeApplet?.Plasmoid.pluginName === "org.kde.plasma.weather"
    readonly property real dashboardWidth: Math.max(1, Math.min(1600, Math.max(260, Plasmoid.configuration.controlCenterWidth ?? 297), Screen.width - 60))
    readonly property real dashboardHeight: Math.max(1, Math.min(1600, Math.max(240, Plasmoid.configuration.controlCenterHeight ?? 451), Screen.height - 60))
    onCompactDashboardChanged: {
        if (!compactDashboard) Qt.callLater(function() {
            if (popup.compactDashboard) return;
            // AppletPopup retains the previous window size when its maximum
            // is released. Restore the detail size from the original dashboard,
            // after the detail page's minimum sizes have taken effect.
            dialog.width = popup.Layout.preferredWidth + dialog.leftPadding + dialog.rightPadding;
            dialog.height = Math.max(popup.Layout.preferredHeight, Math.min(451, Screen.height - 60))
                + dialog.topPadding + dialog.bottomPadding;
        });
    }
    Layout.minimumWidth: compactDashboard ? dashboardWidth : weatherPage ? Math.min(520, Screen.width - 60) : Kirigami.Units.gridUnit * 24
    Layout.preferredWidth: Layout.minimumWidth
    Layout.maximumWidth: compactDashboard ? dashboardWidth : Infinity''')
    text = replace(text, '    Layout.minimumHeight: Kirigami.Units.gridUnit * 24', '''    Layout.minimumHeight: compactDashboard ? dashboardHeight : weatherPage ? Math.min(640, Screen.height - 100) : Kirigami.Units.gridUnit * 24
    Layout.preferredHeight: Layout.minimumHeight
    Layout.maximumHeight: compactDashboard ? Layout.minimumHeight : Infinity''')
    text = replace(text, 'visible: systemTrayState.activeApplet && systemTrayState.activeApplet.expanded && (popup.hiddenLayout.itemCount > 0)', 'visible: !!systemTrayState.activeApplet || popup.showAllItems')
    text = replace(text, '                    systemTrayState.setActiveApplet(null)\n', '                    popup.goHome()\n')
    text = replace(text, 'i18n("Status and Notifications")', '(popup.showAllItems ? i18n("More tray items") : i18n("Control Center"))')
    # Media and other applets can remove their configure action entirely.
    text = replace(text, 'actionsButton.applet?.plasmoid.internalAction("configure").text ?? ""',
                   'actionsButton.applet?.plasmoid.internalAction("configure")?.text ?? ""')
    text = replace(text, '        // Grid view of all available items', '''        ControlDashboard {
            id: dashboard
            backend: controls
            customItems: trayItems.customItems
            otherItems: trayItems.otherItems
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: !systemTrayState.activeApplet && !popup.showAllItems
            onOpenApplet: plugin => popup.openApplet(plugin)
        }

        // Grid view of all available items''')
    text = replace(text, '            visible: !systemTrayState.activeApplet\n', '            visible: !systemTrayState.activeApplet && popup.showAllItems\n')
    text = replace(text, 'import QtQuick.Window', 'import QtQuick.Window\nimport "../details" as Details')
    text = replace(text, 'height: trayHeading.height + bottomPadding + container.headingHeight',
                   '''height: trayHeading.height + 8
        topInset: 0
        bottomInset: 0
        leftInset: container.visible ? container.Layout.leftMargin : dashboard.Layout.leftMargin
        rightInset: container.visible ? container.Layout.rightMargin : dashboard.Layout.rightMargin
        background: Details.DetailSurface { radius: 10; elevated: true }''')
    text = replace(text, 'spacing: plasmoidHeading.bottomPadding', 'spacing: 8')
    text = replace(text, '            id: trayHeading', '''            id: trayHeading
            // A taller applet must not stretch the shared header row.
            Layout.fillHeight: false
            Layout.maximumHeight: implicitHeight
            Layout.topMargin: 4
            Layout.leftMargin: plasmoidHeading.leftInset + 6
            Layout.rightMargin: plasmoidHeading.rightInset + 6
            Layout.bottomMargin: 4
            spacing: 6''')
    text = replace(text, '                level: 1', '''                level: 2
                font.weight: Font.DemiBold
                Layout.fillHeight: true
                verticalAlignment: Text.AlignVCenter
                topPadding: 0
                bottomPadding: 0''')
    text = replace(text, 'Layout.topMargin: mergeHeadings ? 0 : dialog.topPadding', 'Layout.topMargin: 0\n            Layout.margins: 4')
    text = replace(text, 'visible: container.appletHasFooter', 'visible: false // Detail pages own their inset footer surfaces.')
    # Retain every native action and key binding, only replace button surfaces.
    text = text.replace('PlasmaComponents.ToolButton {', 'ControlHeaderButton {')
    text = text.replace('implicitWidth: Kirigami.Units.iconSizes.smallMedium', 'implicitWidth: 18')
    path.write_text(text)
    path = tray / 'PlasmoidPopupsContainer.qml'
    text = replace(path.read_text(), 'import QtQuick\n', 'import QtQuick\nimport "../details" as Details\n')
    text = replace(text, '    focus: true', '''    focus: true
    ControlScrollBars { targetItem: mainStack.currentItem }
    // Also give third-party applets a matching content surface.
    background: Details.DetailSurface {
        objectName: "sequoiaDetailFallback"
        radius: 22
        elevated: true
        visible: mainStack.currentItem !== null
            && mainStack.currentItem.objectName !== "sequoiaDetailPage"
            && mainStack.currentItem.objectName !== "sequoiaWeatherPage"
            && mainStack.currentItem.objectName !== "sequoiaContactsPage"
            && mainStack.activeApplet?.Plasmoid.pluginName !== "org.kde.plasma.mediacontroller"
    }''')
    text = text.replace('activeApplet.fullRepresentationItem.header.background.visible = false',
                        'activeApplet.fullRepresentationItem.header.background.visible = activeApplet.fullRepresentationItem.header.objectName === "sequoiaDetailHeading"')
    text = text.replace('activeApplet.fullRepresentationItem.footer.background.visible = false',
                        'activeApplet.fullRepresentationItem.footer.background.visible = activeApplet.fullRepresentationItem.footer.objectName === "sequoiaDetailHeading"')
    path.write_text(text)
    path = tray / 'main.qml'
    text = replace(path.read_text(), 'visible: root.hiddenLayout.itemCount > 0', 'visible: true // The control center is useful even with no hidden icons.')
    path.write_text(text)
    path = tray / 'ExpanderArrow.qml'
    text = path.read_text().replace('i18n("Show hidden icons")', 'i18n("Control Center")').replace('i18n("Show all the items in the system tray in a popup")', 'i18n("Open Control Center")')
    # Animate the sliders with the native hover and popup state.
    start = text.index('    Kirigami.Icon {')
    text = text[:start] + '''    ControlCenterIcon {
        anchors.centerIn: parent
        width: Math.min(tooltip.iconSize, tooltip.width, tooltip.height)
        height: width
        hovered: tooltip.containsMouse
        expanded: systemTrayState.expanded
    }
}
'''
    path.write_text(text)
    # Round only the artwork in KDE's own expanded media popup. Keep Image as
    # the StackView item so loading, transitions and the blurred backdrop work.
    media_plugin = QPluginLoader(str(MEDIA_PLUGIN))
    if not media_plugin.load():
        raise RuntimeError(media_plugin.errorString())
    media = destination / 'mediacontroller'
    resource = ':/qt/qml/plasma/applet/org/kde/plasma/mediacontroller/'
    files = QDirIterator(resource, QDir.Files, QDirIterator.Subdirectories)
    while files.hasNext():
        name = files.next()
        source = QFile(name)
        assert source.open(QFile.ReadOnly)
        target = media / name.removeprefix(resource)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(bytes(source.readAll()))
    (media / 'qmldir').unlink()
    path = media / 'main.qml'
    text = replace(path.read_text(), 'import plasma.applet.org.kde.plasma.mediacontroller',
                   'import plasma.applet.org.kde.plasma.mediacontroller as NativeMedia')
    text = replace(text, '    GlobalConfig {', '    NativeMedia.GlobalConfig {')
    path.write_text(text)
    path = media / 'AlbumArtStackView.qml'
    text = replace(path.read_text(), 'import QtQuick.Controls',
                   'import QtQuick.Controls\nimport Qt5Compat.GraphicalEffects')
    text = replace(text, '            Image { // Album Art', '''            Image { // Album Art
                id: roundedAlbumArt
                objectName: "sequoiaNativeMediaArtwork"
                layer.enabled: !container.inCompactRepresentation
                    && status === Image.Ready && GraphicsInfo.api !== GraphicsInfo.Software
                layer.effect: OpacityMask {
                    maskSource: Item {
                        width: roundedAlbumArt.width
                        height: roundedAlbumArt.height
                        Rectangle {
                            anchors.centerIn: parent
                            width: Math.min(roundedAlbumArt.paintedWidth, parent.width)
                            height: Math.min(roundedAlbumArt.paintedHeight, parent.height)
                            radius: Math.min(16, width / 2, height / 2)
                            color: "white"
                            antialiasing: true
                        }
                    }
                }''')
    path.write_text(text)
    for source in (SOURCE / 'media').glob('*.qml'):
        shutil.copy2(source, media / source.name)
    dependencies = [PLUGIN, MEDIA_PLUGIN, *(Path(QLibraryInfo.path(QLibraryInfo.LibrariesPath)) / name for name in ('libPlasma.so.7', 'libPlasmaQuick.so.7'))]
    dependencies += style_details(destination, SOURCE, PLUGIN.parent, extract, replace)
    dependencies += style_weather(destination, SOURCE, PLUGIN.parent, extract, replace)
    dependencies += style_contacts(destination, SOURCE, PLUGIN.parent, extract, replace)
    dependencies += style_dock(destination, SOURCE, PLUGIN.parent, extract, replace)
    style_notifications(destination, SOURCE, replace)
    (destination / 'manifest.json').write_text(json.dumps({'fingerprints': {
        str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in dependencies
    }}, indent=2) + '\n')
    (destination / MARKER).write_text('1\n')
    shutil.copy2(SOURCE / 'bridge.cpp', destination / 'bridge.cpp')
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core'], text=True))
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(destination / 'bridge.cpp'), '-o', str(destination / 'tray-qml.so'), *flags, '-ldl'], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--stage', action='store_true', help='Build only; do not configure the desktop service')
    parser.add_argument('--remove', action='store_true')
    args = parser.parse_args()
    target = args.prefix.resolve() / 'sequoia-eleven/control-center'
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    dropin = config / 'systemd/user/plasma-plasmashell.service.d/50-sequoia-control-center.conf'
    if args.remove:
        if not (target / MARKER).exists():
            raise RuntimeError('No managed Control Center installation found.')
        if dropin.exists():
            if '# Sequoia Eleven Control Center' not in dropin.read_text():
                raise RuntimeError('Service override was modified; review before removing.')
            dropin.unlink()
        shutil.rmtree(target)
        subprocess.run(['systemctl', '--user', 'daemon-reload'], check=True)
        print('Removed Control Center. Restart plasma-plasmashell.service to restore the original tray.')
        return
    if target.exists() and not (target / MARKER).exists():
        raise RuntimeError(f'Refusing to replace an unrelated directory: {target}')
    if not args.stage and dropin.exists() and '# Sequoia Eleven Control Center' not in dropin.read_text():
        raise RuntimeError(f'Refusing to replace an unrelated service override: {dropin}')
    # Build and check every integration anchor before touching the installation.
    with tempfile.TemporaryDirectory(prefix='sequoia-control-center-') as temporary:
        stage = Path(temporary) / 'controlcenter'
        build(stage)
        shutil.copy2(SOURCE / 'launch.py', stage / 'launch.py')
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            shutil.rmtree(target)
        shutil.copytree(stage, target)
    if not args.stage:
        dropin.parent.mkdir(parents=True, exist_ok=True)
        launcher = str(target / 'launch.py').replace('%', '%%').replace('"', '\\"')
        dropin.write_text('# Sequoia Eleven Control Center\n[Service]\nExecStart=\n'
                          f'ExecStart=/usr/bin/python3 "{launcher}" /usr/bin/plasmashell --no-respawn\n')
        subprocess.run(['systemctl', '--user', 'daemon-reload'], check=True)
    print(f'Installed Control Center: {target}')
    print('Restart plasma-plasmashell.service to load it. Remove with: python3 tools/install-control-center.py --remove')


if __name__ == '__main__':
    main()
