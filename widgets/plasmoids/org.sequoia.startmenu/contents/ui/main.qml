// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    objectName: "sequoiaStartMenu"
    preferredRepresentation: fullRepresentation
    Plasmoid.icon: Plasmoid.configuration.icon
    Plasmoid.title: i18n("Sequoia Start Menu")
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    toolTipMainText: i18n("Start")
    toolTipSubText: i18n("Apps, files, and settings")
    readonly property alias menuOpen: popup.opened
    readonly property bool verticalPanel: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    // The custom taskbar supplies the same effective size used by its tasks.
    // Ordinary Plasma panels retain the launcher's panel-based sizing.
    readonly property real taskbarIconSize: parent?.taskbarIconSize ?? 0
    Layout.minimumWidth: verticalPanel ? 32 : 48
    Layout.preferredWidth: verticalPanel ? 48 : height
    Layout.maximumWidth: verticalPanel ? Infinity : Math.max(48, height)
    Layout.minimumHeight: 32
    Layout.preferredHeight: verticalPanel ? width : 48

    PopupPlacement {
        id: placement
        anchor: root
        screenRect: root.screenGeometry.width > 0 ? root.screenGeometry
            : Qt.rect(root.Screen.virtualX, root.Screen.virtualY, root.Screen.width, root.Screen.height)
        location: Plasmoid.location
        requestedWidth: Plasmoid.configuration.menuWidth
        requestedHeight: Plasmoid.configuration.menuHeight
    }

    MenuBackend { id: dataBackend; configuration: Plasmoid.configuration; appletInterface: root }
    function closeMenu() { popup.opened = false; }
    function toggleMenu() { popup.opened = !popup.opened; }
    Plasmoid.onActivated: toggleMenu()
    fullRepresentation: PC.ToolButton {
        objectName: "startMenuButton"
        implicitWidth: 48; implicitHeight: 48
        Layout.minimumWidth: 48
        Layout.preferredWidth: root.verticalPanel ? 48 : root.height
        padding: 0
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0
        contentItem: Item {
            Kirigami.Icon {
                objectName: "startMenuLauncherIcon"
                anchors.centerIn: parent
                width: root.taskbarIconSize > 0
                    ? Math.min(root.taskbarIconSize, parent.width, parent.height)
                    : Math.max(0, Math.min(parent.width, parent.height) - 4)
                height: width
                source: Plasmoid.configuration.icon
                active: root.menuOpen
            }
        }
        checkable: true
        checked: root.menuOpen
        Accessible.name: i18n("Open Start menu")
        onClicked: root.toggleMenu()
    }
    StartMenuPopup {
        id: popup
        // Keep the transient relationship, but own the geometry: visualParent
        // would make Dialog reposition itself above the launcher on each resize.
        transientParent: root.Window.window
        x: placement.popupX
        y: placement.popupY
        popupWidth: placement.popupWidth
        popupHeight: placement.popupHeight
        configuration: Plasmoid.configuration
        backend: dataBackend
        onSettingsRequested: {
            const action = Plasmoid.internalAction("configure");
            if (action) action.trigger();
        }
    }
}
