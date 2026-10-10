// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: motion
    required property bool animationsEnabled
    required property bool hovered
    required property bool pressed
    required property bool launching
    required property bool launchEnabled
    required property bool pressEnabled
    required property int hoverStyle
    required property int strength
    required property int speed
    property real direction: -1
    readonly property bool moving: animationsEnabled && Kirigami.Units.longDuration > 0
    readonly property int duration: moving ? Math.round(Kirigami.Units.shortDuration * 100 / Math.max(25, speed)) : 0
    readonly property real amount: Math.max(0, Math.min(20, strength)) / 100
    property real bounce: 0
    property real zoom: !moving ? 1 : pressed && pressEnabled ? 0.94
        : hovered && hoverStyle !== 1 ? 1 + amount : 1
    property real lift: moving && hovered && hoverStyle !== 0 ? direction * amount * 24 : 0
    readonly property real offset: lift + direction * bounce
    Behavior on zoom { NumberAnimation { duration: motion.duration; easing.type: Easing.OutCubic } }
    Behavior on lift { NumberAnimation { duration: motion.duration; easing.type: Easing.OutCubic } }
    SequentialAnimation {
        running: motion.moving && motion.launchEnabled && motion.launching
        loops: 3
        onStopped: motion.bounce = 0
        NumberAnimation { target: motion; property: "bounce"; to: motion.amount * 45; duration: motion.duration; easing.type: Easing.OutQuad }
        NumberAnimation { target: motion; property: "bounce"; to: 0; duration: motion.duration * 1.5; easing.type: Easing.OutBounce }
    }
}
