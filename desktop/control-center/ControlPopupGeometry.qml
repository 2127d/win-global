// SPDX-License-Identifier: MIT
import QtQuick

// Measurements are queued before they reach native window constraints. Animate
// only an already-open popup; opening must not inherit the last page's size.
QtObject {
    id: geometry
    property real targetWidth: 418
    property real targetHeight: 380
    property bool active: false
    property int duration: 200
    property real width: targetWidth
    property real height: targetHeight
    property bool ready: false
    readonly property bool running: widthMotion.running || heightMotion.running
    function schedule() { if (active) settle.restart(); }
    onTargetWidthChanged: schedule()
    onTargetHeightChanged: schedule()
    onDurationChanged: schedule()
    onActiveChanged: {
        ready = false;
        widthMotion.stop(); heightMotion.stop(); settle.stop();
        if (active) settle.restart();
    }
    function retarget(animation, name, value) {
        if (Math.abs((animation.running ? animation.to : geometry[name]) - value) < 0.5) return;
        animation.stop();
        animation.from = geometry[name];
        animation.to = value;
        animation.start();
    }
    readonly property Timer settle: Timer {
        interval: 16
        onTriggered: {
            if (!geometry.active) return;
            if (!geometry.ready || geometry.duration <= 0) {
                widthMotion.stop(); heightMotion.stop();
                geometry.width = geometry.targetWidth;
                geometry.height = geometry.targetHeight;
                geometry.ready = true;
            } else {
                geometry.retarget(widthMotion, "width", geometry.targetWidth);
                geometry.retarget(heightMotion, "height", geometry.targetHeight);
            }
        }
    }
    readonly property NumberAnimation widthMotion: NumberAnimation {
        target: geometry; property: "width"; duration: geometry.duration; easing.type: Easing.OutCubic
    }
    readonly property NumberAnimation heightMotion: NumberAnimation {
        target: geometry; property: "height"; duration: geometry.duration; easing.type: Easing.OutCubic
    }
}
