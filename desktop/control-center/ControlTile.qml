// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

ControlGlassSurface {
    id: tile
    required property string title
    required property string subtitle
    required property var symbol
    property bool maskIcon: true
    property bool checkable: true
    property bool selected: false
    property bool available: true
    property bool hasDetails: true
    readonly property bool iconOnly: width < 95
    readonly property bool vertical: !iconOnly && height > width * 0.7
    readonly property bool showArrow: hasDetails && !iconOnly && width >= 150
    signal triggered()
    signal detailsRequested()
    implicitHeight: Math.max(60, titles.implicitHeight + 20)
    radius: iconOnly ? Math.min(width, height) / 2 : vertical ? 24 : height / 2
    emphasized: selected
    readonly property color foreground: Kirigami.Theme.textColor

    PC.AbstractButton {
        id: toggle
        property bool held: false
        objectName: tile.title + "Toggle"
        anchors.fill: parent
        enabled: tile.available
        text: tile.title
        Accessible.name: tile.title + ", " + tile.subtitle
        Accessible.role: tile.checkable ? Accessible.CheckBox : Accessible.Button
        Accessible.checkable: tile.checkable
        Accessible.checked: tile.selected
        onPressed: held = false
        onClicked: if (!held) tile.triggered()
        onPressAndHold: { held = true; tile.detailsRequested(); }
        background: Rectangle {
            radius: tile.radius
            color: Qt.alpha(tile.foreground, toggle.hovered ? 0.09 : 0)
            border.width: toggle.activeFocus ? 2 : 0
            border.color: tile.foreground
        }
        contentItem: Item {
            opacity: tile.available ? 1 : 0.5
            Rectangle {
                id: iconDisc
                x: tile.iconOnly || tile.vertical ? (parent.width - width) / 2 : 10
                y: tile.vertical ? 14 : (parent.height - height) / 2
                width: tile.width < 150 && !tile.iconOnly && !tile.vertical ? 24 : 32
                height: width; radius: width / 2
                color: tile.selected ? Kirigami.Theme.highlightColor : Qt.alpha(tile.foreground, 0.08)
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: 18; height: 18
                    source: tile.symbol
                    color: tile.selected ? Kirigami.Theme.highlightedTextColor : tile.foreground
                    isMask: tile.maskIcon
                    selected: tile.selected
                }
            }
            Column {
                id: titles
                visible: !tile.iconOnly
                x: tile.vertical ? 10 : iconDisc.x + iconDisc.width + 8
                y: tile.vertical ? 54 : (parent.height - implicitHeight) / 2
                width: Math.max(0, parent.width - x - (tile.showArrow ? 30 : 10))
                spacing: 0
                PC.Label { width: parent.width; text: tile.title; font.weight: Font.DemiBold; color: tile.foreground; elide: Text.ElideRight }
                PC.Label { visible: tile.width >= 150 || tile.vertical; width: parent.width; text: tile.subtitle; color: tile.foreground; opacity: 0.78; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9; maximumLineCount: 1; elide: Text.ElideRight; textFormat: Text.PlainText }
            }
        }
        PC.ToolTip { text: tile.title + ": " + tile.subtitle }
    }
    PC.ToolButton {
        anchors.right: parent.right; anchors.rightMargin: 6; anchors.verticalCenter: parent.verticalCenter
        width: 24; height: Math.min(48, tile.height - 12)
        visible: tile.showArrow
        text: qsTr("%1 details").arg(tile.title)
        display: PC.AbstractButton.IconOnly
        background: Rectangle {
            radius: width / 2
            color: Qt.alpha(tile.foreground, parent.down ? 0.12 : parent.hovered ? 0.08 : 0)
            border.width: parent.activeFocus ? 2 : 0
            border.color: Kirigami.Theme.highlightColor
        }
        contentItem: Kirigami.Icon { source: "go-next-symbolic"; color: tile.foreground; selected: tile.selected; implicitWidth: 16; implicitHeight: 16; isMask: true }
        onClicked: tile.detailsRequested()
        PC.ToolTip { text: parent.text }
    }
}
