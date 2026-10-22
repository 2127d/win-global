// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts

Item {
    id: guard
    required property var container
    required property var layout
    property bool adjusting: false
    property bool queued: false
    function schedule() {
        if (!adjusting && !queued && container.configOverlayVisible && !container.dragActive) {
            queued = true;
            Qt.callLater(snap);
        }
    }
    function snap() {
        queued = false;
        if (!container || !layout || adjusting || container.dragActive) return;
        adjusting = true;
        const cell = GridSession.cell;
        const w = Math.max(cell, Math.ceil(container.Layout.minimumWidth / cell) * cell,
                           Math.round(container.width / cell) * cell);
        const h = Math.max(cell, Math.ceil(container.Layout.minimumHeight / cell) * cell,
                           Math.round(container.height / cell) * cell);
        const x = Math.max(0, Math.round(container.x / cell) * cell);
        const y = Math.max(0, Math.round(container.y / cell) * cell);
        layout.releaseSpace(container);
        if (x + w <= layout.width && y + h <= layout.height && layout.isRectAvailable(x, y, w, h)) {
            geometryChange.apply(container, Qt.rect(x, y, w, h));
        }
        // Re-register occupancy after snapping. Native resize handlers release
        // their own space again before checking the next pointer movement.
        layout.positionItem(container);
        adjusting = false;
    }
    Connections {
        target: guard.container
        function onWidthChanged() { guard.schedule(); }
        function onHeightChanged() { guard.schedule(); }
        // Plasma moves the container by incremental pointer deltas. Snapping
        // x/y during a move consumes those deltas and pins slow drags in place.
        // Its native move/drop path already snaps to the layout's grid and
        // updates occupancy. Only resizing needs our live geometry correction.
    }
    GridGeometry { id: geometryChange }
}
