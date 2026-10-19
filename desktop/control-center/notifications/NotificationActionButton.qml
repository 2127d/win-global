// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PC.ToolButton {
    id: control
    topPadding: 7
    bottomPadding: 7
    leftPadding: 12
    rightPadding: 12
    font.family: Kirigami.Theme.defaultFont.family
    font.pointSize: Kirigami.Theme.defaultFont.pointSize
    font.weight: Font.Medium
    background: Rectangle {
        radius: 9
        color: Qt.alpha(Kirigami.Theme.highlightColor, control.down ? 0.22 : control.hovered ? 0.17 : 0.10)
        border.width: control.visualFocus ? 2 : 1
        border.color: Qt.alpha(Kirigami.Theme.highlightColor, control.visualFocus ? 1 : 0.13)
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }
}
