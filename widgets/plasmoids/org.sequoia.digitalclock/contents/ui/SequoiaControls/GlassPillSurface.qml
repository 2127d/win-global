// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as K

// A translucent optical surface over the host's existing blurred backdrop.
// Text and input live above this surface and retain their full opacity.
K.ShadowedRectangle {
    id: surface
    property color surfaceColor: K.Theme.backgroundColor
    property color foregroundColor: K.Theme.textColor
    property bool selected: false
    readonly property bool dark: surfaceColor.hslLightness < .5
    radius: height / 2
    color: Qt.alpha(surfaceColor, selected ? (dark ? .58 : .46) : (dark ? .30 : .24))
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, selected ? (dark ? .22 : .64) : (dark ? .12 : .38))
    shadow.size: selected ? 5 : 2
    shadow.yOffset: selected ? 2 : 1
    shadow.color: Qt.rgba(0, 0, 0, dark ? .16 : .08)
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, surface.radius - 1)
        antialiasing: true
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, surface.dark ? .12 : .30) }
            GradientStop { position: .48; color: "transparent" }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, surface.dark ? .07 : .035) }
        }
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, surface.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: Qt.alpha(surface.foregroundColor, .05)
        antialiasing: true
    }
}
