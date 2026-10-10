// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtCore
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    objectName: "sequoiaDownloadsStack"
    // This widget owns its popup, so its full representation is the dock button.
    // A compact-only applet has no representation for Plasma to display here.
    preferredRepresentation: fullRepresentation
    Layout.minimumWidth: 48
    Layout.preferredWidth: 56
    Layout.maximumWidth: 56
    Layout.minimumHeight: 32
    Plasmoid.icon: "folder-downloads"
    Plasmoid.title: i18n("Downloads")
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    toolTipMainText: i18n("Downloads")
    toolTipSubText: i18n("Show recent downloads")
    readonly property url downloadsUrl: Plasmoid.configuration.folderUrl.toString() || StandardPaths.writableLocation(StandardPaths.DownloadLocation)
    property bool stackOpen: false
    property url pendingUrl

    fullRepresentation: PC.ToolButton {
        objectName: "downloadsStackButton"
        Accessible.name: i18n("Show downloads")
        Layout.minimumWidth: 48
        Layout.maximumWidth: 56
        implicitWidth: 56
        implicitHeight: 56
        padding: 0
        // Plasma's ToolButton sets each edge independently of padding.
        topPadding: 0
        bottomPadding: 0
        leftPadding: 0
        rightPadding: 0
        contentItem: Item {
            Kirigami.Icon {
                objectName: "downloadsFolderIcon"
                anchors.centerIn: parent
                width: Math.min(48, parent.width, parent.height)
                height: width
                // Keep the existing Sequoia folder artwork at small dock sizes.
                source: Qt.resolvedUrl("../icons/downloads.svg")
            }
        }
        checkable: true
        checked: fanDialog.visible
        onClicked: root.toggleFan()
    }
    function toggleFan() {
        if (fanDialog.visible) {
            closeFan();
        } else {
            pendingUrl = "";
            stackOpen = true;
            fanDialog.hadFocus = false;
            fanDialog.visible = true;
            fan.opened = true;
            fanDialog.requestActivate();
        }
    }
    function closeFan(url) {
        if (!stackOpen) return;
        pendingUrl = url || "";
        stackOpen = false;
        fan.opened = false;
    }
    Plasmoid.onActivated: toggleFan()
    PlasmaCore.Dialog {
        id: fanDialog
        objectName: "downloadsFanWindow"
        property bool sequoiaNoBlur: true
        property bool closeOnFocusLoss: true
        property bool hadFocus: false
        visible: false
        visualParent: root.fullRepresentationItem
        location: Plasmoid.location
        type: PlasmaCore.Dialog.PopupMenu
        // Keep the surface alive until the files have folded into the folder.
        hideOnWindowDeactivate: false
        backgroundHints: PlasmaCore.Dialog.NoBackground
        onActiveChanged: {
            if (active) { hadFocus = true; fan.focusNewest(); }
            else if (visible && hadFocus && closeOnFocusLoss) root.closeFan();
        }
        onVisibleChanged: {
            if (visible) Qt.callLater(fan.focusNewest);
            else { root.stackOpen = false; fan.opened = false; }
        }
        mainItem: FanView {
            id: fan
            focus: true
            folderUrl: root.downloadsUrl
            opened: false
            maximumHeight: Math.max(180, Screen.height - 150)
            width: implicitWidth
            height: implicitHeight
            onFileActivated: url => root.closeFan(url)
            onFolderRequested: root.closeFan(root.downloadsUrl)
            onDismissed: root.closeFan()
            onFolded: {
                if (root.stackOpen) return;
                fanDialog.visible = false;
                const destination = root.pendingUrl;
                root.pendingUrl = "";
                if (destination.toString()) Qt.openUrlExternally(destination);
            }
        }
    }
}
