// SPDX-License-Identifier: MIT
import QtQuick

// PropertyAction deliberately bypasses the desktop container's drop animation.
// Otherwise positionItem() reads the old position while x/y are still animating.
SequentialAnimation {
    id: move
    property var container: null
    property rect bounds
    PropertyAction { target: move.container; property: "x"; value: move.bounds.x }
    PropertyAction { target: move.container; property: "y"; value: move.bounds.y }
    PropertyAction { target: move.container; property: "width"; value: move.bounds.width }
    PropertyAction { target: move.container; property: "height"; value: move.bounds.height }
    function apply(item, rect) {
        stop();
        container = item;
        bounds = rect;
        start();
    }
}
