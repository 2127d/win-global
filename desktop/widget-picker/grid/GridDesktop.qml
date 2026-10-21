// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

Item {
    id: grid
    objectName: "sequoiaDesktopGrid"
    required property var containment
    required property var view
    readonly property var layout: containment?.appletsLayout ?? null
    parent: layout
    anchors.fill: parent
    z: 10000
    readonly property string kind: "desktop"
    readonly property int screen: containment?.plasmoid?.screen ?? -1
    readonly property rect geometry: Qt.rect(view.x, view.y, view.width, view.height)
    readonly property string label: qsTr("Desktop · Screen %1").arg(screen + 1)
    readonly property bool ready: layout !== null && screen >= 0
        && containment.plasmoid.immutability === PlasmaCore.Types.Mutable
    readonly property bool placing: GridSession.phase === "placement" && GridSession.destination === grid
    readonly property bool dragging: GridSession.phase === "drag"
    readonly property bool active: ready && (placing || dragging)
    readonly property int columns: Math.floor(width / GridSession.cell)
    readonly property int rows: Math.floor(height / GridSession.cell)
    property int hoverColumn: 0
    property int hoverRow: 0
    property bool pointerInside: false
    property int revision: 0
    property var guards: []
    readonly property int spanX: GridSession.preset.columns
    readonly property int spanY: GridSession.preset.rows
    readonly property bool valid: revision >= 0 && available(hoverColumn, hoverRow)

    Binding { target: grid.layout; property: "cellWidth"; value: GridSession.cell; when: grid.layout !== null }
    Binding { target: grid.layout; property: "cellHeight"; value: GridSession.cell; when: grid.layout !== null }

    function available(column, row) {
        return ready && column >= 0 && row >= 0 && column + spanX <= columns && row + spanY <= rows
            && layout.isRectAvailable(column * GridSession.cell, row * GridSession.cell,
                                      spanX * GridSession.cell, spanY * GridSession.cell);
    }
    function pointAt(x, y) {
        hoverColumn = Math.floor(x / GridSession.cell);
        hoverRow = Math.floor(y / GridSession.cell);
        pointerInside = true;
        revision++;
    }
    function activatePlacement() {
        containment.plasmoid.corona.editMode = false;
        forceActiveFocus();
        view.requestActivate();
        revision++;
    }
    function placeAt(column, row) {
        if (!available(column, row)) return false;
        const rect = Qt.rect(column * GridSession.cell, row * GridSession.cell,
                             spanX * GridSession.cell, spanY * GridSession.cell);
        const applet = GridSession.addApplet(containment, GridSession.plugin);
        if (!applet || applet.failedToLaunch) {
            GridSession.error = qsTr("The widget could not be loaded.");
            return false;
        }
        Qt.callLater(() => {
            const container = layout.children.find(c => c.applet && c.applet.plasmoid.id === applet.id);
            if (container) {
                layout.releaseSpace(container);
                geometryChange.apply(container, rect);
                layout.positionItem(container);
            }
            attachGuards();
            layout.save();
        });
        if (!dragging) GridSession.complete();
        revision++;
        return true;
    }
    function attachGuards() {
        if (!layout) return;
        for (const child of layout.children) {
            if (child.applet && GridSession.supports(child.applet.plasmoid.pluginName)
                    && !guards.some(g => g && g.container === child)) {
                guards = guards.concat([guardFactory.createObject(child, {container: child, layout: layout})]);
            }
        }
        guards = guards.filter(g => g && g.container);
        revision++;
    }
    Component { id: guardFactory; GridItem {} }
    GridGeometry { id: geometryChange }
    Connections {
        target: grid.layout
        function onChildrenChanged() { Qt.callLater(grid.attachGuards); }
    }
    onLayoutChanged: Qt.callLater(attachGuards)
    Component.onCompleted: { GridSession.registerTarget(grid); Qt.callLater(attachGuards); }
    Component.onDestruction: GridSession.unregisterTarget(grid)

    Rectangle { anchors.fill: parent; visible: grid.active; color: "#38000000" }
    Repeater {
        model: grid.active ? grid.columns * grid.rows : 0
        Rectangle {
            required property int index
            readonly property int column: index % grid.columns
            readonly property int row: Math.floor(index / grid.columns)
            x: column * GridSession.cell + 4; y: row * GridSession.cell + 4
            width: GridSession.cell - 8; height: width; radius: 12
            color: "transparent"
            border.width: 1
            border.color: grid.revision >= 0 && grid.available(column, row) ? "#65ffffff" : "#20ffffff"
        }
    }
    Rectangle {
        visible: grid.active && (grid.placing || grid.pointerInside)
        x: grid.hoverColumn * GridSession.cell + 6; y: grid.hoverRow * GridSession.cell + 6
        width: grid.spanX * GridSession.cell - 12; height: grid.spanY * GridSession.cell - 12
        radius: 18
        color: Qt.alpha(grid.valid ? Kirigami.Theme.highlightColor : Kirigami.Theme.negativeTextColor, 0.3)
        border.width: 2
        border.color: grid.valid ? Kirigami.Theme.highlightColor : Kirigami.Theme.negativeTextColor
        PC3.Label {
            anchors.centerIn: parent
            text: "%1 × %2".arg(grid.spanX).arg(grid.spanY)
            font.pixelSize: 22
            color: "white"
            style: Text.Outline
            styleColor: "#90000000"
        }
    }
    MouseArea {
        anchors.fill: parent
        enabled: grid.placing
        hoverEnabled: true
        onPositionChanged: mouse => grid.pointAt(mouse.x, mouse.y)
        onClicked: mouse => { grid.pointAt(mouse.x, mouse.y); grid.placeAt(grid.hoverColumn, grid.hoverRow); }
    }
    DropArea {
        anchors.fill: parent
        enabled: grid.dragging && grid.ready
        keys: ["text/x-plasmoidservicename"]
        onEntered: drag => {
            drag.accepted = drag.getDataAsString("text/x-plasmoidservicename") === GridSession.plugin;
            if (drag.accepted) grid.pointAt(drag.x, drag.y);
        }
        onPositionChanged: drag => grid.pointAt(drag.x, drag.y)
        onExited: grid.pointerInside = false
        onDropped: drop => {
            grid.pointAt(drop.x, drop.y);
            if (grid.placeAt(grid.hoverColumn, grid.hoverRow)) drop.acceptProposedAction();
            else drop.accepted = false;
            grid.pointerInside = false;
        }
    }
    Rectangle {
        visible: grid.placing
        anchors.horizontalCenter: parent.horizontalCenter
        y: 12; width: Math.min(parent.width - 24, 540); height: 52; radius: 16
        color: Kirigami.Theme.backgroundColor
        PC3.Label { anchors.centerIn: parent; text: qsTr("Click to place · Arrow keys to move · Enter to place · Esc to cancel"); font.pixelSize: 12 }
    }
    Keys.onPressed: event => {
        if (!placing) return;
        if (event.key === Qt.Key_Escape) GridSession.cancel();
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) placeAt(hoverColumn, hoverRow);
        else if (event.key === Qt.Key_Left) hoverColumn = Math.max(0, hoverColumn - 1);
        else if (event.key === Qt.Key_Right) hoverColumn = Math.min(columns - spanX, hoverColumn + 1);
        else if (event.key === Qt.Key_Up) hoverRow = Math.max(0, hoverRow - 1);
        else if (event.key === Qt.Key_Down) hoverRow = Math.min(rows - spanY, hoverRow + 1);
        else return;
        event.accepted = true;
    }
}
