// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.ExpandableListItem {
    id: control
    property bool grouped:false
    DetailSurface {
        objectName: "sequoiaDetailCard"
        visible:!control.grouped || control.expanded || control.ListView.isCurrentItem || control.activeFocus || hover.hovered
        radius:control.grouped && !control.expanded ? height/2 : 24
        anchors.fill: parent
        z: -1
        emphasized: control.expanded || control.ListView.isCurrentItem
        focused: control.activeFocus
        hovered: hover.hovered
    }
    HoverHandler { id: hover }
}
