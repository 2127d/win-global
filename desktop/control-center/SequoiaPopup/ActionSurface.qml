// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

Surface {
    property bool raised: false
    radius: Tokens.controlRadius
    color: raised ? stateColor : Qt.alpha(Kirigami.Theme.highlightColor,
        emphasized ? Tokens.selectedOpacity : pressed ? Tokens.pressedOpacity : hovered ? Tokens.hoverOpacity : 0)
    elevated: raised
    border.width: focused ? Tokens.focusWidth : 0
}
