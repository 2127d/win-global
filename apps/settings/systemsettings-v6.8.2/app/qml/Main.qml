// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as K
Rectangle {
    color: appearancePreview.enabled ? appearancePreview.canvasColor : "transparent"
    objectName: "settingsSidebarLoader"
    readonly property real islandInset: appearancePreview.enabled ? 10 : 0
    implicitWidth: (sidebar.item ? sidebar.item.implicitWidth : 256) + islandInset * 2
    implicitHeight: sidebar.item ? sidebar.item.implicitHeight : 640
    readonly property real headerHeight: sidebar.item ? sidebar.item.headerHeight : 60
    Loader { id: sidebar; anchors.fill: parent; anchors.margins: islandInset; source: appearancePreview.enabled ? "HubSidebar.qml" : "LegacyMain.qml" }
}
