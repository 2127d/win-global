// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.ToolButton {
    id: control
    padding: 4
    topPadding: 2
    bottomPadding: 2
    icon.width: Tokens.iconSize
    icon.height: Tokens.iconSize
    implicitWidth: Math.max(30, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(28, implicitContentHeight + topPadding + bottomPadding)
    background: ActionSurface {
        emphasized: control.checked
        pressed: control.down
        hovered: control.hovered && control.enabled
        focused: control.visualFocus
    }
}
