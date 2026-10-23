// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
ScrollBar {
    id: bar
    property color tint: Kirigami.Theme.textColor
    implicitWidth: horizontal ? 100 : 6
    implicitHeight: horizontal ? 6 : 100
    padding: 1
    visible: size < 1
    background: Item {}
    contentItem: Rectangle {
        implicitWidth: 4; implicitHeight: 4; radius: 2
        color: bar.tint
        opacity: bar.active || bar.pressed ? 0.5 : 0.16
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
    }
}
