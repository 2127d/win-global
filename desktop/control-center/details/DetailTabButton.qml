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
        color: Kirigami.Theme.textColor
    }
    background: DetailSurface {
        radius: height / 2
        emphasized: control.checked
        hovered: control.hovered
        pressed: control.down
        focused: control.activeFocus
    }
}
