// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts

Item {
    id: guard
    required property var container
    required property var layout
    readonly property bool automaticSize: container.applet?.sequoiaAutomaticSize ?? false
    property var handleGuards: []
    readonly property var overlay: container.configOverlayItem
    property var touchArea: null
    function guardHandles() {
        touchArea = overlay?.children.find(child => child.maximumTouchPoints !== undefined) ?? null;
        if (!automaticSize || !overlay) return;
        for (const handle of overlay.children) {
            if (handle.resizeCorner !== undefined && !handleGuards.some(g => g && g.nativeHandle === handle)) {
                handleGuards = handleGuards.concat([handleGuardFactory.createObject(handle, {nativeHandle: handle})]);
            }
        }
        handleGuards = handleGuards.filter(g => g && g.nativeHandle);
    }
    Component { id: handleGuardFactory; FixedSizeHandleGuard {} }
    // Preserve the overlay's one-finger move gesture, without two-finger scaling.
    Binding {
        target: guard.touchArea
        property: "maximumTouchPoints"
        value: 1
        when: guard.automaticSize && guard.touchArea !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
    onOverlayChanged: Qt.callLater(guardHandles)
    property bool adjusting: false
    property bool queued: false
    function schedule() {
        if (!adjusting && !queued && (automaticSize || container.configOverlayVisible) && !container.dragActive) {
            queued = true;
            Qt.callLater(snap);
        }
    }
    function snap() {
        queued = false;
        if (!container || !layout || adjusting || container.dragActive) return;
        adjusting = true;
        const cell = GridSession.cell;
        const w = Math.min(container.Layout.maximumWidth,
                           Math.max(cell, Math.ceil(container.Layout.minimumWidth / cell) * cell,
                                    Math.round(container.width / cell) * cell));
        const h = Math.min(container.Layout.maximumHeight,
                           Math.max(cell, Math.ceil(container.Layout.minimumHeight / cell) * cell,
                                    Math.round(container.height / cell) * cell));
        const x = Math.max(0, Math.round(container.x / cell) * cell);
        const y = Math.max(0, Math.round(container.y / cell) * cell);
        layout.releaseSpace(container);
        if (automaticSize || (x + w <= layout.width && y + h <= layout.height && layout.isRectAvailable(x, y, w, h))) {
            geometryChange.apply(container, Qt.rect(x, y, w, h));
        }
        // Re-register occupancy after snapping. Native resize handlers release
        // their own space again before checking the next pointer movement.
        layout.positionItem(container);
        adjusting = false;
    }
    Connections {
        target: guard.container
        function onWidthChanged() { if (!guard.automaticSize) guard.schedule(); }
        function onHeightChanged() { if (!guard.automaticSize) guard.schedule(); }
        function onDragActiveChanged() { if (guard.automaticSize) guard.schedule(); }
        // Plasma moves the container by incremental pointer deltas. Snapping
        // x/y during a move consumes those deltas and pins slow drags in place.
        // Its native move/drop path already snaps to the layout's grid and
        // updates occupancy. Only resizing needs our live geometry correction.
    }
    Connections {
        target: guard.container.applet?.Layout ?? null
        function onMinimumWidthChanged() { guard.schedule(); }
        function onMinimumHeightChanged() { guard.schedule(); }
        function onMaximumWidthChanged() { guard.schedule(); }
        function onMaximumHeightChanged() { guard.schedule(); }
    }
    onAutomaticSizeChanged: schedule()
    Component.onCompleted: { schedule(); Qt.callLater(guardHandles); }
    GridGeometry { id: geometryChange }
}
