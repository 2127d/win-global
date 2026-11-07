// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami

WeatherGlassSurface {
    id: surface
    property bool elevated: false
    readonly property color flatSurface: Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.55))
    // A reading tint lets scenery through while supporting the brighter
    // forecast captions. Rim intensity remains independent of this fill.
    fillOpacity: 1 - Math.max(0, Math.min(100, atmospheric
        ? settingsModel.cardTransparency : settingsModel.flatCardTransparency)) / 100
    tint: atmospheric ? "#172638" : flatSurface
    radius: 24
    // A stronger lens in the outer 18 px; the shader's center has zero offset.
    refractionStrength: 28
    bevelWidth: 18
    shadow: false
    RectangularShadow {
        objectName: "weatherCardShadow"
        anchors.fill: parent
        z: -1
        visible: GraphicsInfo.api !== GraphicsInfo.Software
        radius: surface.radius
        blur: 20
        offset: Qt.vector2d(0, 6)
        color: Qt.rgba(0, 0, 0, surface.atmospheric ? 0.30 : 0.22)
    }
}
