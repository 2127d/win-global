// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami.platform as Platform

// Painted inside the control's allocation: no input interception or extra window.
Item {
    id: root
    objectName: "sequoiaIsland"
    property real inset: 5
    property real radius: 14
    property color color: Platform.Theme.backgroundColor
    // Surface opacity is independent of the surrounding application canvas.
    readonly property color surfaceColor: Qt.rgba(color.r, color.g, color.b, 1)
    readonly property bool dark: Platform.Theme.textColor.hslLightness > 0.5
    readonly property alias border: surface.border

    Rectangle {
        x: root.inset - 1
        y: root.inset + 2
        width: Math.max(0, root.width - 2 * root.inset + 2)
        height: Math.max(0, root.height - 2 * root.inset)
        radius: root.radius + 1
        color: root.dark ? "#18000000" : "#0c000000"
    }
    Rectangle {
        x: root.inset
        y: root.inset + 1
        width: Math.max(0, root.width - 2 * root.inset)
        height: Math.max(0, root.height - 2 * root.inset)
        radius: root.radius
        color: root.dark ? "#22000000" : "#14000000"
    }
    Rectangle {
        id: surface
        anchors.fill: parent
        anchors.margins: root.inset
        radius: Math.min(root.radius, width / 2, height / 2)
        color: root.surfaceColor
        border.width: Platform.Theme.frameContrast > 0 ? 1 : 0
        border.color: root.dark ? "#1cffffff" : "#18000000"
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.lighter(root.surfaceColor, root.dark ? 1.10 : 1.015) }
            GradientStop { position: 1; color: root.surfaceColor }
        }
    }
}
