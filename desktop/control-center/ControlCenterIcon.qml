// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Shapes
import org.kde.kirigami as Kirigami

Item {
    id: icon
    property color color: Kirigami.Theme.textColor
    property bool hovered: false
    property bool expanded: false
    // The knobs approach on hover, swap on opening, and return on closing.
    // Kirigami durations follow Plasma's animation-speed setting, including Off.
    readonly property real targetPosition: expanded ? 1 : hovered ? 0.16 : 0
    property real switchPosition: targetPosition
    Behavior on switchPosition {
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.InOutCubic
        }
    }
    // Keep the two switches square and centered at every panel size.
    Item {
        anchors.centerIn: parent
        width: 24
        height: 24
        scale: Math.min(icon.width, icon.height) / 24
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: -1
                fillColor: icon.color
                fillRule: ShapePath.OddEvenFill
                // Upper switch: fine outline and a moving solid knob.
                // Lower switch: solid capsule with a moving transparent knob,
                // so the actual panel tint shows through on either theme.
                PathSvg {
                    path: "M7 3 H17 A4 4 0 0 1 17 11 H7 A4 4 0 0 1 7 3 Z "
                        + "M7 4.4 H17 A2.6 2.6 0 0 1 17 9.6 H7 A2.6 2.6 0 0 1 7 4.4 Z "
                        + "M" + (8.7 + 10 * icon.switchPosition) + " 7 a1.7 1.7 0 1 1 -3.4 0 a1.7 1.7 0 1 1 3.4 0 Z "
                        + "M7 13 H17 A4 4 0 0 1 17 21 H7 A4 4 0 0 1 7 13 Z "
                        + "M" + (18.8 - 10 * icon.switchPosition) + " 17 a1.8 1.8 0 1 1 -3.6 0 a1.8 1.8 0 1 1 3.6 0 Z"
                }
            }
        }
    }
}
