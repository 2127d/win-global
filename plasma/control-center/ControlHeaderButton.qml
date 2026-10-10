// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

PC.ToolButton {
    id: control
    padding: 4
    topPadding: 2
    bottomPadding: 2
    icon.width: 18
    icon.height: 18
    implicitWidth: Math.max(30, implicitContentWidth + 8)
    implicitHeight: Math.max(24, implicitContentHeight + 4)
    background: Rectangle {
        radius: 10
        color: Qt.alpha(Kirigami.Theme.highlightColor, control.checked ? 0.18 : control.down ? 0.14 : control.hovered ? 0.09 : 0)
        border.width: control.activeFocus ? 2 : 0
        border.color: Kirigami.Theme.highlightColor
    }
}
