// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Effects
import org.kde.kirigami as Kirigami

Kirigami.Icon {
    id: icon
    property bool glowActive: false
    property bool glowAnimationsEnabled: true
    property real glowStrength: glowActive ? 1 : 0
    readonly property bool glowRendered: layer.enabled

    Behavior on glowStrength {
        NumberAnimation {
            duration: icon.glowAnimationsEnabled && Kirigami.Units.longDuration > 1 ? 140 : 0
            easing.type: Easing.OutCubic
        }
    }

    // A centered, colored shadow follows the icon's alpha contour. Applying it
    // to the icon's layer keeps the glow attached during zoom/lift animations.
    // The software scene graph cannot render shaders; keep the icon visible.
    layer.enabled: glowStrength > 0 && GraphicsInfo.api !== GraphicsInfo.Software
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: Qt.tint(Kirigami.Theme.highlightColor, "#60ffffff")
        shadowOpacity: 0.85 * icon.glowStrength
        shadowBlur: 0.7
        blurMax: Math.max(6, Math.min(16, Math.round(icon.width / 4)))
        shadowScale: 1.04
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 0
        autoPaddingEnabled: true
    }
}
