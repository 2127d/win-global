// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

Rectangle {
    id: tile
    required property string title
    required property string subtitle
    required property var symbol
    property bool maskIcon: true
    property bool checkable: true
    property bool selected: false
    property bool available: true
    property bool hasDetails: true
    signal triggered()
    signal detailsRequested()
    implicitHeight: Math.max(64, titles.implicitHeight + 36)
    radius: 15
    color: selected ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor.r < 0.5 ? Qt.tint(Kirigami.Theme.backgroundColor, "#0dffffff") : Qt.tint(Kirigami.Theme.backgroundColor, "#88ffffff")
    border.width: 1
    border.color: Qt.alpha(selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor, 0.08)
    readonly property color foreground: selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }

    PC.AbstractButton {
        id: toggle
        objectName: tile.title + "Toggle"
        anchors.fill: parent
        anchors.rightMargin: tile.hasDetails ? 24 : 0
        enabled: tile.available
        text: tile.title
        Accessible.name: tile.title + ", " + tile.subtitle
        Accessible.role: tile.checkable ? Accessible.CheckBox : Accessible.Button
        Accessible.checkable: tile.checkable
        Accessible.checked: tile.selected
        onClicked: tile.triggered()
        background: Rectangle {
            radius: 15
            color: Qt.alpha(tile.foreground, toggle.hovered ? 0.09 : 0)
            border.width: toggle.activeFocus ? 2 : 0
            border.color: tile.foreground
        }
        contentItem: Item {
            opacity: tile.available ? 1 : 0.5
            Kirigami.Icon {
                x: 10; y: 8; width: 18; height: 18
                source: tile.symbol
                color: tile.foreground
                isMask: tile.maskIcon
                selected: tile.selected
            }
            Column {
                id: titles
                x: 10; y: 30
                width: parent.width - 14
                spacing: 0
                PC.Label { width: parent.width; text: tile.title; font.weight: Font.DemiBold; color: tile.foreground; elide: Text.ElideRight }
                PC.Label { width: parent.width; text: tile.subtitle; color: tile.foreground; opacity: 0.78; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9; maximumLineCount: 1; elide: Text.ElideRight; textFormat: Text.PlainText }
            }
        }
        PC.ToolTip { text: tile.title + ": " + tile.subtitle }
    }
    PC.ToolButton {
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
        width: 24; height: 48
        visible: tile.hasDetails
        text: qsTr("%1 details").arg(tile.title)
        display: PC.AbstractButton.IconOnly
        contentItem: Kirigami.Icon { source: "go-next-symbolic"; color: tile.foreground; selected: tile.selected; implicitWidth: 16; implicitHeight: 16; isMask: true }
        onClicked: tile.detailsRequested()
        PC.ToolTip { text: parent.text }
    }
}
