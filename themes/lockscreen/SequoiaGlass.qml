// SPDX-License-Identifier: MIT
import QtQuick
import Qt5Compat.GraphicalEffects

Rectangle {
    id: glass
    required property Item sceneSource
    required property Item scene
    required property var themeColors
    property real blurRadius: 32
    readonly property bool accelerated: GraphicsInfo.api !== GraphicsInfo.Software
    // mapToItem is reevaluated when the scene or panel geometry changes.
    readonly property rect sampleRect: {
        const geometry = [x, y, width, height, scene.width, scene.height];
        const point = mapToItem(sceneSource, 0, 0);
        return Qt.rect(point.x, point.y, width, height);
    }
    radius: 18
    color: accelerated ? themeColors.glass : themeColors.surface
    ShaderEffectSource {
        id: sample
        visible: false
        sourceItem: glass.sceneSource
        sourceRect: glass.sampleRect
        textureSize: Qt.size(glass.width, glass.height)
        live: true
    }
    FastBlur {
        anchors.fill: parent
        source: sample
        radius: glass.blurRadius
        visible: glass.accelerated
        layer.enabled: glass.accelerated
        layer.effect: OpacityMask {
            maskSource: Rectangle { width: glass.width; height: glass.height; radius: glass.radius }
        }
    }
    Rectangle {
        anchors.fill: parent
        radius: glass.radius
        color: glass.accelerated ? glass.themeColors.glass : "transparent"
        border.color: glass.themeColors.border
    }
}
