// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Controls.AbstractButton {
    id: entry
    property string iconName
    property real tilt: 0
    property real iconSize: 40
    property real labelLimit: 180
    property real foldScale: 1
    property real labelReveal: 1
    implicitWidth: labelLimit + iconSize + 12
    implicitHeight: Math.max(iconSize + 4, label.implicitHeight + 12)
    padding: 0
    focusPolicy: Qt.StrongFocus
    Accessible.name: text
    // Keep labels horizontal; only the icon tilts along the curved stack.
    transform: Scale { origin.x: entry.width - entry.iconSize / 2; origin.y: entry.height / 2; xScale: entry.foldScale; yScale: entry.foldScale }
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    contentItem: Item {
        Rectangle {
            anchors.right: fileIcon.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.ceil(Math.min(entry.labelLimit, label.implicitWidth + 14))
            height: Math.ceil(label.implicitHeight + 8)
            radius: 4
            readonly property color backingColor: entry.hovered || entry.visualFocus ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
            color: Qt.rgba(backingColor.r, backingColor.g, backingColor.b, 0.96)
            opacity: entry.labelReveal
            border.width: 1
            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.12)
            Controls.Label {
                id: label
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideMiddle
                text: entry.text
                font: Kirigami.Theme.defaultFont
                renderType: Text.NativeRendering
                color: entry.hovered || entry.visualFocus ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
            }
        }
        Kirigami.Icon {
            id: fileIcon
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: entry.iconSize
            height: width
            source: entry.iconName
            rotation: entry.tilt
            scale: entry.hovered || entry.visualFocus ? 1.08 : 1
            Behavior on scale { NumberAnimation { duration: Kirigami.Units.shortDuration } }
        }
    }
    Controls.ToolTip.visible: hovered
    Controls.ToolTip.text: text
    Controls.ToolTip.delay: 700
}
