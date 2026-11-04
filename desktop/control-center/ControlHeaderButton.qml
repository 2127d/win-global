// SPDX-License-Identifier: MIT
import "SequoiaPopup" as Popup
import org.kde.plasma.components as PC

Popup.GlassToolButton {
    padding: 4
    topPadding: 2
    bottomPadding: 2
    icon.width: Popup.Tokens.iconSize
    icon.height: Popup.Tokens.iconSize
    implicitWidth: display === PC.AbstractButton.IconOnly ? implicitHeight : Math.max(32, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
}
