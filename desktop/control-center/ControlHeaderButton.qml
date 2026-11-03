// SPDX-License-Identifier: MIT
import "SequoiaPopup" as Popup

Popup.GlassToolButton {
    padding: 4
    topPadding: 2
    bottomPadding: 2
    icon.width: Popup.Tokens.iconSize
    icon.height: Popup.Tokens.iconSize
    implicitWidth: Math.max(30, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(28, implicitContentHeight + topPadding + bottomPadding)
}
