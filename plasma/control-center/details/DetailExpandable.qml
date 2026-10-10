// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.ExpandableListItem {
    id: control
    DetailSurface {
        objectName: "sequoiaDetailCard"
        anchors.fill: parent
        z: -1
        emphasized: control.expanded || control.ListView.isCurrentItem
        focused: control.activeFocus
        hovered: hover.hovered
    }
    HoverHandler { id: hover }
}
