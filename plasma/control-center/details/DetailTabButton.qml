// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

PC.TabButton {
    id: control
    padding: 9
    leftInset: 3
    rightInset: 3
    contentItem: PC.Label {
        text: control.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.weight: control.checked ? Font.DemiBold : Font.Normal
        color: control.checked ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
    }
    background: Rectangle {
        radius: height / 2
        color: control.checked ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, control.hovered ? 0.09 : 0.04)
        border.width: control.activeFocus ? 2 : 0
        border.color: Kirigami.Theme.textColor
    }
}
