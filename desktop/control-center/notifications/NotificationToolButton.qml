// SPDX-License-Identifier: MIT
import QtQuick
import "../details" as Details
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
    background: Details.DetailSurface {
        radius: height / 2
        pressed: control.down
        hovered: control.hovered
        focused: control.visualFocus
        visible: control.down || control.hovered || control.visualFocus
    }
}
