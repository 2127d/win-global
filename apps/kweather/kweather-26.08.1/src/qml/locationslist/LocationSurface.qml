// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.sequoia.glass as Glass
import org.kde.kirigami as Kirigami

Kirigami.ShadowedRectangle {
    property bool hovered: false
    property bool pressed: false
    property bool focused: false
    radius: 22
    color: Qt.tint(Kirigami.Theme.backgroundColor,
        Qt.alpha(Kirigami.Theme.textColor, pressed ? 0.10 : hovered ? 0.04 : 0))
    border.width: focused ? 2 : color.a < 1 ? 0 : 1
    border.color: focused ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.13)
    Glass.Rim { anchors.fill: parent; radius: parent.radius; visible: parent.color.a < 1 }
    shadow.size: Kirigami.Units.largeSpacing
    shadow.color: Qt.alpha(Kirigami.Theme.textColor, 0.10)
    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
}
