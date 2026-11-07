// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami

WeatherGlassSurface {
    id: surface
    property bool elevated: false
    readonly property color flatSurface: Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.55))
    // Denser than the controls so small forecast labels remain legible even
    // over the brightest scenery. The rim setting never changes this tint.
    fillOpacity: atmospheric ? (elevated ? 0.78 : 0.72) : 0.80
    tint: atmospheric ? "#172638" : flatSurface
    radius: 24
    refractionStrength: 18
    bevelWidth: 16
    shadow: false
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
}
