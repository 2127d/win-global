// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "QuickSettingsLayout.js" as Tiles

FocusScope {
    id: frame
    required property var controller
    required property var placement
    required property Item grid
    objectName: "tileFrame:" + placement.id
    readonly property bool selected: controller.selectedId === placement.id
    Accessible.name: placement.spec.title
    Accessible.description: controller.editing ? qsTr("Tile. Use the toolbar to move, resize or remove it.") : ""
    activeFocusOnTab: controller.editing
    onActiveFocusChanged: if (activeFocus && controller.editing) controller.selectedId = placement.id
    Keys.onDeletePressed: if (controller.editing) controller.removeTile(placement.id)
    Keys.onLeftPressed: event => { if (controller.editing) controller.moveSelected(-1); else event.accepted = false; }
    Keys.onRightPressed: event => { if (controller.editing) controller.moveSelected(1); else event.accepted = false; }
    Keys.onSpacePressed: event => { if (controller.editing) controller.cycleSize(placement.id); else event.accepted = false; }

    QuickSettingsContent {
        id: tileContent
        anchors.fill: parent
        controller: frame.controller
        spec: frame.placement.spec
        enabled: !frame.controller.editing
        opacity: frame.controller.editing ? 0.45 : 1
    }
    Rectangle {
        anchors.fill: parent
        radius: Math.min(24, height / 2)
        visible: frame.controller.editing
        color: Qt.alpha(Kirigami.Theme.highlightColor, frame.selected ? 0.08 : 0.02)
        border.width: frame.selected || frame.controller.dropId === frame.placement.id ? 2 : 1
        border.color: frame.selected || frame.controller.dropId === frame.placement.id ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)
    }
    MouseArea {
        id: moveArea
        objectName: "moveHandle:" + frame.placement.id
        anchors.fill: parent
        enabled: frame.controller.editing
        preventStealing: true
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property real startX: 0
        property real startY: 0
        property bool moved: false
        onPressed: mouse => {
            frame.controller.selectedId = frame.placement.id;
            frame.forceActiveFocus(); startX = mouse.x; startY = mouse.y; moved = false;
        }
        onPositionChanged: mouse => {
            if (!pressed) return;
            if (Math.abs(mouse.x - startX) + Math.abs(mouse.y - startY) > 8) moved = true;
            if (moved) {
                const point = mapToItem(frame.grid, mouse.x, mouse.y);
                frame.controller.updateDrag(frame.placement.id, point.x, point.y);
            }
        }
        onReleased: if (moved) frame.controller.finishDrag()
        onCanceled: frame.controller.endGesture()
    }
    PC.ToolButton {
        objectName: "removeTile:" + frame.placement.id
        visible: frame.controller.editing
        anchors.top: parent.top; anchors.right: parent.right
        width: 26; height: 26
        text: qsTr("Remove %1").arg(frame.placement.spec.title)
        Accessible.name: text
        display: PC.AbstractButton.IconOnly
        icon.name: "list-remove"
        background: Rectangle { radius: width / 2; color: Kirigami.Theme.backgroundColor; border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2); border.width: 1 }
        onClicked: frame.controller.removeTile(frame.placement.id)
        PC.ToolTip { text: parent.text }
    }
    PC.Label {
        visible: frame.controller.editing && frame.width >= 90
        anchors.bottom: parent.bottom; anchors.left: parent.left
        anchors.leftMargin: 10; anchors.bottomMargin: 4
        text: frame.placement.savedW + " × " + frame.placement.savedH
        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.85
        color: Kirigami.Theme.textColor
    }
    Rectangle {
        id: resizeGrip
        objectName: "resizeGrip:" + frame.placement.id
        visible: frame.controller.editing
        anchors.right: parent.right; anchors.bottom: parent.bottom
        width: 30; height: 24; radius: 10
        color: frame.selected ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
        border.color: Kirigami.Theme.highlightColor; border.width: 1
        Kirigami.Icon { anchors.centerIn: parent; width: 16; height: 16; source: "transform-scale"; isMask: true; color: frame.selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor }
        MouseArea {
            objectName: "resizeHandle:" + frame.placement.id
            anchors.fill: parent
            preventStealing: true
            cursorShape: Qt.SizeFDiagCursor
            property real startX: 0
            property real startY: 0
            property bool moved: false
            onPressed: mouse => {
                frame.controller.selectedId = frame.placement.id;
                const point = mapToItem(frame.grid, mouse.x, mouse.y);
                startX = point.x; startY = point.y; moved = false;
            }
            onPositionChanged: mouse => {
                if (!pressed) return;
                const point = mapToItem(frame.grid, mouse.x, mouse.y);
                if (Math.abs(point.x - startX) + Math.abs(point.y - startY) > 8) moved = true;
                if (moved) frame.controller.updateResize(frame.placement.id, point.x - startX, point.y - startY);
            }
            onReleased: {
                if (moved) frame.controller.finishResize();
                else frame.controller.cycleSize(frame.placement.id);
            }
            onCanceled: frame.controller.endGesture()
        }
    }
}
