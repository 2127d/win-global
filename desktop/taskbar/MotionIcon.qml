// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

GlowIcon {
    id: icon
    required property var settings
    required property bool hovered
    required property bool pressed
    property bool keyboardFocus: false
    glowActive: hovered || pressed || keyboardFocus
    glowAnimationsEnabled: settings.dockAnimations
    property bool launching: false
    property bool demandingAttention: false
    property bool horizontal: true
    property real inwardDirection: -1
    // Full button dimensions, independent of its visual content and motion.
    required property real viewportWidth
    required property real viewportHeight
    readonly property bool moving: motion.moving
    readonly property int motionDuration: motion.duration
    readonly property real crossRoom: Math.max(0, ((horizontal ? viewportHeight : viewportWidth)
        - (horizontal ? height : width) * scale) / 2)
    readonly property real motionOffset: Math.max(-crossRoom, Math.min(crossRoom, motion.offset))
    property real attentionPulse: 1
    scale: Math.min(motion.zoom, viewportWidth / Math.max(1, width), viewportHeight / Math.max(1, height))
    transform: Translate {
        x: icon.horizontal ? 0 : icon.motionOffset
        y: icon.horizontal ? icon.motionOffset : 0
    }
    DockMotion {
        id: motion
        animationsEnabled: icon.settings.dockAnimations
        hovered: icon.hovered && icon.settings.taskHoverEffect
        pressed: icon.pressed
        launching: icon.launching
        launchEnabled: icon.settings.dockLaunchAnimation
        pressEnabled: icon.settings.dockPressAnimation
        hoverStyle: icon.settings.dockHoverStyle
        strength: icon.settings.dockStrength
        speed: icon.settings.dockSpeed
        direction: icon.inwardDirection
    }
    SequentialAnimation {
        running: motion.moving && icon.settings.dockAttentionAnimation && icon.demandingAttention
        loops: 1
        onStopped: icon.attentionPulse = 1
        NumberAnimation { target: icon; property: "attentionPulse"; to: 0.35; duration: 225; easing.type: Easing.InOutSine }
        NumberAnimation { target: icon; property: "attentionPulse"; to: 1; duration: 225; easing.type: Easing.InOutSine }
    }
}
