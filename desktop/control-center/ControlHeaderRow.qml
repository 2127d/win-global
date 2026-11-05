// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

RowLayout {
    id: row
    readonly property int verticalInset: 4

    function focusNeighbour(step) {
        const controls = [];
        function collect(item) {
            if (!item.visible || !item.enabled) return;
            if (item instanceof T.AbstractButton) {
                controls.push(item);
            } else {
                for (const child of item.children) collect(child);
            }
        }
        for (const child of row.children) collect(child);
        // Include loaded widget controls and repeated actions in visual order, also in RTL.
        const center = item => item.mapToItem(row, item.width / 2, 0).x;
        controls.sort((a, b) => center(a) - center(b));
        const index = controls.findIndex(item => item.activeFocus);
        if (index < 0) return false;
        controls[index + step]?.forceActiveFocus(Qt.TabFocusReason);
        return true;
    }

    Keys.onLeftPressed: event => { event.accepted = row.focusNeighbour(-1); }
    Keys.onRightPressed: event => { event.accepted = row.focusNeighbour(1); }
}
