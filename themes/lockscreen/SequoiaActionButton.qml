// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PC.Button {
    id: control
    readonly property real unit: lockScreenUi.powerUnit
    readonly property var appearance: lockScreenUi.themeColors
    implicitWidth: 60 * unit
    implicitHeight: 56 * unit
    padding: 0
    hoverEnabled: true
    Accessible.name: text
    background: Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 34 * control.unit; height: width
        radius: width / 2
        color: control.down ? Qt.alpha(control.appearance.foreground, 0.22) : control.hovered ? Qt.alpha(control.appearance.foreground, 0.12) : control.appearance.field
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? control.appearance.focusColor : control.appearance.border
    }
    contentItem: Item {
        opacity: control.enabled ? 1 : 0.4
        Kirigami.Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 7 * control.unit
            width: 20 * control.unit; height: width
            source: control.icon.name
            color: control.appearance.foreground
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 39 * control.unit
            text: control.text.replace(/&/g, "")
            color: control.appearance.foreground
            font.pixelSize: Math.max(10, 10 * control.unit)
        }
    }
    Keys.onEnterPressed: clicked()
    Keys.onReturnPressed: clicked()
}
