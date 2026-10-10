// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PC.ToolButton {
    id: control
    implicitWidth: 28
    implicitHeight: 28
    topPadding: 6
    bottomPadding: 6
    leftPadding: 6
    rightPadding: 6
    icon.width: 16
    icon.height: 16
    background: Rectangle {
        radius: width / 2
        color: Qt.alpha(Kirigami.Theme.textColor, control.down ? 0.13 : control.hovered ? 0.08 : 0)
        border.width: control.visualFocus ? 2 : 0
        border.color: Kirigami.Theme.highlightColor
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }
}
