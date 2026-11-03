// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

// Let the existing blurred popup show through the tile. Only the material is
// translucent; labels, icons and keyboard focus retain their full contrast.
Surface {
    id: glassTile
    glass: true
    color: Qt.alpha(stateColor, dark ? 0.58 : 0.46)
    border.width: focused ? Tokens.focusWidth : 1
    border.color: focused ? Kirigami.Theme.highlightColor : Qt.rgba(1, 1, 1, dark ? 0.22 : 0.64)
    shadow.size: elevated ? 12 : 5
    shadow.yOffset: elevated ? 3 : 2
    shadow.color: Qt.rgba(0, 0, 0, dark ? 0.16 : 0.08)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, glassTile.radius - 1)
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, glassTile.dark ? 0.12 : 0.30) }
            GradientStop { position: 0.48; color: "transparent" }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, glassTile.dark ? 0.07 : 0.035) }
        }
        antialiasing: true
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, glassTile.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, glassTile.dark ? 0.05 : 0.13)
        antialiasing: true
    }
}
