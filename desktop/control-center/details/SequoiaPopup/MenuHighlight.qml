// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

// Accent-tinted glass over the menu's existing backdrop. Keep the material
// inside the row so scrolling and rounded popup clipping remain unchanged.
Rectangle {
    id: menuGlass
    property bool pressed: false
    property bool focused: false
    property color accentColor: Kirigami.Theme.highlightColor
    readonly property bool dark: Kirigami.Theme.backgroundColor.hslLightness < 0.5
    radius: 13
    antialiasing: true
    color: Qt.alpha(accentColor, pressed ? 0.30 : 0.20)
    border.width: 1
    border.color: focused ? accentColor : Qt.alpha(accentColor, dark ? 0.60 : 0.42)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, menuGlass.radius - 1)
        antialiasing: true
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, menuGlass.dark ? 0.16 : 0.38) }
            GradientStop { position: 0.48; color: Qt.alpha(menuGlass.accentColor, 0.03) }
            GradientStop { position: 1; color: Qt.alpha(menuGlass.accentColor, menuGlass.dark ? 0.16 : 0.12) }
        }
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, menuGlass.dark ? 0.10 : 0.25)
    }
}
