// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.components as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.AbstractButton {
    id: tile
    objectName: "startAppTile"
    required property var menu
    required property var sourceModel
    required property int entryIndex
    required property var entry
    property bool listStyle: false
    property bool allowPin: true
    readonly property string favoriteId: entry?.favoriteId || ""
    readonly property string description: entry?.description || ""
    property bool launching: false
    text: entry?.display || ""
    implicitHeight: listStyle ? 56 : 82
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: text
    Accessible.description: description
    enabled: !launching && !menu.launching
    scale: launching && menu.configuration.shrinkOnLaunch ? 0.78 : (down ? 0.95 : 1)
    Behavior on scale { NumberAnimation { duration: tile.menu.motionDuration / 2; easing.type: Easing.OutCubic } }
    onClicked: {
        launching = true;
        menu.launch(sourceModel, entryIndex);
        releaseTimer.restart();
    }
    Timer { id: releaseTimer; interval: tile.menu.motionDuration + 80; onTriggered: tile.launching = false }
    background: Rectangle {
        radius: 7
        color: tile.hovered || tile.down ? tile.menu.hoverColor : "transparent"
        border.width: tile.activeFocus ? 2 : 0
        border.color: tile.menu.accentColor
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    contentItem: Item {
        Kirigami.Icon {
            id: appIcon
            source: tile.entry?.decoration || "application-x-executable"
            width: tile.listStyle ? 30 : 32
            height: width
            x: tile.listStyle ? 12 : (parent.width - width) / 2
            y: tile.listStyle ? (parent.height - height) / 2 : 9
        }
        Text {
            x: tile.listStyle ? 54 : 3
            y: tile.listStyle ? (tile.description ? 10 : (parent.height - height) / 2) : 48
            width: parent.width - x - (tile.listStyle ? 12 : 3)
            text: tile.text
            color: tile.menu.textColor
            font.pixelSize: tile.listStyle ? 13 : 11
            horizontalAlignment: tile.listStyle ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
            maximumLineCount: tile.listStyle ? 1 : 2
            wrapMode: tile.listStyle ? Text.NoWrap : Text.Wrap
        }
        Text {
            visible: tile.listStyle && tile.description.length > 0
            x: 54; y: 29; width: parent.width - 66
            text: tile.description
            color: tile.menu.secondaryColor
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }
    QQC2.ToolTip.visible: hovered && !contextMenu.opened
    QQC2.ToolTip.delay: 800
    QQC2.ToolTip.text: description ? text + "\n" + description : text
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: if (tile.allowPin && tile.favoriteId) contextMenu.popup()
    }
    Keys.onMenuPressed: if (allowPin && favoriteId) contextMenu.popup()
    QQC2.Menu {
        id: contextMenu
        QQC2.MenuItem {
            text: tile.menu.backend.isPinned(tile.favoriteId) ? qsTr("Unpin from Start") : qsTr("Pin to Start")
            icon.name: "pin"
            onTriggered: tile.menu.backend.togglePin(tile.favoriteId)
        }
        QQC2.MenuItem {
            text: qsTr("Move earlier")
            visible: tile.menu.backend.isPinned(tile.favoriteId)
            height: visible ? implicitHeight : 0
            onTriggered: tile.menu.backend.movePin(tile.favoriteId, -1)
        }
        QQC2.MenuItem {
            text: qsTr("Move later")
            visible: tile.menu.backend.isPinned(tile.favoriteId)
            height: visible ? implicitHeight : 0
            onTriggered: tile.menu.backend.movePin(tile.favoriteId, 1)
        }
    }
}
