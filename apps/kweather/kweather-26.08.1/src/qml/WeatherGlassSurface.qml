// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.sequoia.glass as Glass

Rectangle {
    id: surface
    property bool atmospheric: false
    property bool hovered: false
    property bool pressed: false
    property bool focused: false
    property bool shadow: true
    property real fillOpacity: 0.22
    property real refractionStrength: 12
    property real bevelWidth: 8
    property color tint: atmospheric ? "#142235" : Kirigami.Theme.backgroundColor
    readonly property bool prismatic: settingsModel.prismaticGlass
    readonly property bool refracting: atmospheric && GraphicsInfo.api !== GraphicsInfo.Software
    // Neutral density preserves contrast without dyeing the transmitted scene.
    readonly property real tintLuminance: tint.r * 0.2126 + tint.g * 0.7152 + tint.b * 0.0722
    readonly property color materialTint: prismatic
        ? Qt.rgba(tintLuminance, tintLuminance, tintLuminance, 1) : tint
    readonly property color surfaceTint: Qt.alpha(materialTint, Math.min(1, fillOpacity + (pressed ? 0.18 : hovered ? 0.09 : 0)))
    radius: height / 2
    // One tint layer: the shader replaces the sampled scene only inside the pill.
    color: refracting ? "transparent" : surfaceTint
    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    RectangularShadow {
        anchors.fill: parent; z: -1
        visible: surface.shadow && GraphicsInfo.api !== GraphicsInfo.Software
        radius: surface.radius; blur: 12; offset: Qt.vector2d(0, 4)
        color: "#3d000000"
    }
    ShaderEffectSource {
        id: backdrop
        visible: false
        // Keep the provider alive through Flat/Dynamic and modal transitions.
        // Only scenery is sampled; foreground controls never enter this pass.
        sourceItem: GraphicsInfo.api !== GraphicsInfo.Software ? applicationWindow().background.scenery : null
        property bool withinViewport: false
        live: surface.visible && surface.refracting && withinViewport && applicationWindow().visibility !== Window.Minimized
        hideSource: false
        smooth: true
        function updateSourceRect() {
            if (!sourceItem || !surface.visible) return;
            const point = surface.mapToItem(sourceItem, 0, 0);
            sourceRect = Qt.rect(point.x, point.y, surface.width, surface.height);
            withinViewport = point.x + surface.width > 0 && point.y + surface.height > 0
                && point.x < sourceItem.width && point.y < sourceItem.height;
        }
        onSourceItemChanged: Qt.callLater(updateSourceRect)
    }
    Connections {
        target: surface.visible ? surface.Window.window : null
        function onAfterAnimating() { Qt.callLater(backdrop.updateSourceRect); }
    }
    ShaderEffect {
        anchors.fill: parent
        visible: surface.refracting
        property var source: backdrop
        property vector2d surfaceSize: Qt.vector2d(width, height)
        property real cornerRadius: surface.radius
        property real refractionStrength: surface.refractionStrength
        property real bevelWidth: surface.bevelWidth
        property real dispersion: surface.prismatic ? 1.0 : 0.0
        fragmentShader: "qrc:/sequoia-weather/weather-glass.frag.qsb"
    }
    Rectangle {
        anchors.fill: parent; radius: surface.radius
        visible: surface.refracting
        color: surface.surfaceTint
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }
    Glass.Rim { anchors.fill: parent; radius: surface.radius }
    ShaderEffect {
        objectName: "weatherPrismaticRim"
        anchors.fill: parent
        visible: surface.prismatic && GraphicsInfo.api !== GraphicsInfo.Software
        property vector2d surfaceSize: Qt.vector2d(width, height)
        property real cornerRadius: surface.radius
        property real rimIntensity: Glass.Settings.intensity
        property var source: backdrop
        property real backdropAvailable: surface.refracting ? 1.0 : 0.0
        fragmentShader: "qrc:/sequoia-weather/weather-prismatic-rim.frag.qsb"
    }
    Rectangle {
        anchors.fill: parent; anchors.margins: 2; radius: Math.max(0, surface.radius - 2)
        visible: surface.focused
        color: "transparent"; border.width: 2; border.color: Kirigami.Theme.highlightColor
    }
}
