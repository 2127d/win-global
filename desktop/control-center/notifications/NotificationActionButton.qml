// SPDX-License-Identifier: MIT
import QtQuick
import "../details" as Details
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
    background: Details.DetailSurface {
        radius: height / 2
        pressed: control.down
        hovered: control.hovered
        focused: control.visualFocus
    }
}
