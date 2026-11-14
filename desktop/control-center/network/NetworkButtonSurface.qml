// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    required property var control
    radius: Math.min(width, height) / 2
    visible: !control.flat || control.hovered || control.down || control.checked || control.visualFocus
    shadow.size: control.flat ? 0 : 5
    hovered: control.hovered && control.enabled
    pressed: control.down
    emphasized: control.checked
    color: control.checked ? Qt.alpha(Kirigami.Theme.highlightColor,0.24) : Qt.alpha(stateColor,dark?0.58:0.46)
    border.color: control.checked ? Qt.alpha(Kirigami.Theme.highlightColor,0.5) : Qt.rgba(1,1,1,dark?0.22:0.64)
    focused: control.visualFocus
}
