// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PC.ScrollBar {
    id: bar
    implicitWidth: 8
    implicitHeight: 8
    padding: 2
    leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
    background: Item {}
    contentItem: Rectangle {
        implicitWidth: 4; implicitHeight: 4
        radius: 2
        color: Kirigami.Theme.textColor
        opacity: bar.pressed ? 0.55 : bar.hovered || bar.active ? 0.40 : 0.16
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration > 1 ? Kirigami.Units.shortDuration : 0 } }
    }
}
