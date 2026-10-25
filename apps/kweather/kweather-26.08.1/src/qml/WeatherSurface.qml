// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Rectangle {
    id: surface
    property bool atmospheric: false
    property bool elevated: false
    readonly property color flatSurface: Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.55))
    RectangularShadow {
        objectName: "weatherCardShadow"
        anchors.fill: parent
        z: -1
        visible: GraphicsInfo.api !== GraphicsInfo.Software
        radius: surface.radius
        blur: 18
        offset: Qt.vector2d(0, 5)
        color: Qt.rgba(0, 0, 0, surface.atmospheric ? 0.24 : 0.16)
    }
    radius: 24
    color: atmospheric ? (elevated ? "#e0192c42" : "#c518293e") : flatSurface
    border.width: 1
    border.color: atmospheric ? "#20ffffff" : Qt.alpha(Kirigami.Theme.textColor, 0.08)
    Behavior on color { ColorAnimation { duration: Kirigami.Units.longDuration } }
}
