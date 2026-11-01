// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Rectangle {
    id: surface
    // Weather/Gwenview: 24px panels, 22px cards, fine rims and soft shadows.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    property bool emphasized: false
    property bool focused: false
    property bool hovered: false
    property bool pressed: false
    property bool elevated: false
    property bool panel: false
    property int motionDuration: Kirigami.Units.shortDuration > 1 ? Kirigami.Units.shortDuration : 0
    radius: panel || elevated ? 24 : 22
    readonly property color base: panel
        ? Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.55))
        : Kirigami.Theme.backgroundColor
    color: Qt.tint(base, emphasized ? Qt.alpha(Kirigami.Theme.highlightColor, 0.16)
        : Qt.alpha(Kirigami.Theme.textColor, pressed ? 0.10 : hovered ? 0.04 : 0))
    border.width: focused ? 2 : 1
    border.color: focused ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, panel ? 0.08 : 0.13)
    antialiasing: true
    RectangularShadow {
        anchors.fill: parent
        z: -1
        visible: surface.elevated && GraphicsInfo.api !== GraphicsInfo.Software
        radius: surface.radius
        blur: surface.panel ? 18 : 12
        offset: Qt.vector2d(0, surface.panel ? 5 : 4)
        color: Qt.rgba(0, 0, 0, surface.panel ? 0.16 : 0.20)
    }
    Behavior on color { ColorAnimation { duration: surface.motionDuration } }
}
