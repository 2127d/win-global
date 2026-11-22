// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Effects as Effects
import org.kde.kirigami as Kirigami

Item {
    id: glass
    property Item wallpaper: null
    property real radius: Math.min(width, height) / 2
    readonly property rect sampleRect: {
        // mapToItem alone does not notify when an ancestor moves.
        const position = glass.Kirigami.ScenePosition;
        const origin = wallpaper?.Kirigami.ScenePosition;
        return wallpaper ? Qt.rect(position.x - origin.x, position.y - origin.y, width, height) : Qt.rect(0, 0, width, height);
    }
    readonly property bool backdropReady: wallpaper !== null && visible
    Loader {
        anchors.fill: parent
        active: glass.backdropReady && GraphicsInfo.api !== GraphicsInfo.Software
        sourceComponent: Effects.MultiEffect {
            autoPaddingEnabled: false
            blurEnabled: true
            blurMax: 32
            blur: 0.5
            source: ShaderEffectSource {
                sourceItem: glass.wallpaper
                sourceRect: glass.sampleRect
                live: glass.backdropReady
            }
            layer.enabled: true
            layer.effect: ShaderEffect {
                // Qt supplies the blurred wallpaper texture to the layer effect.
                property var source
                property vector2d surfaceSize: Qt.vector2d(width, height)
                property real cornerRadius: glass.radius
                property real refraction: 16
                property real bevelWidth: Math.min(18, cornerRadius * 0.6)
                fragmentShader: Qt.resolvedUrl("pure-glass.frag.qsb")
            }
        }
    }
    // The same fine bright outer / dark inner rim used by Sequoia menus.
    // Neither rectangle contributes a surface fill or a light/dark tint.
    Rectangle {
        anchors.fill: parent
        radius: glass.radius
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.19)
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, glass.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.08)
    }
}
