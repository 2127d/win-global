// SPDX-License-Identifier: MIT
import "details" as Details
import QtQuick
import "SequoiaPopup" as Popup
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "QuickSettingsLayout.js" as Tiles

ColumnLayout {
    id: dashboard
    objectName: "quickSettingsDashboard"
    required property var backend
    property var customItems: ["org.kde.plasma.networkmanagement", "org.kde.plasma.bluetooth", "org.kde.plasma.notifications", "org.kde.plasma.volume", "org.kde.plasma.brightness", "org.kde.plasma.mediacontroller"]
    property var otherItems: []
    property var availableCustomItems: customItems
    property var availableOtherItems: otherItems
    property string layoutJson: ""
    signal layoutCommitted(string json)
    signal openApplet(string plugin)
    property bool editing: false
    property var draft: []
    property string selectedId: ""
    readonly property real gridGap: 10
    readonly property real gridWidth: Math.min(scroll.availableWidth, 482)
    readonly property int columns: gridWidth >= 362 ? 6 : 4
    readonly property real unit: (gridWidth - (columns - 1) * gridGap) / columns
    readonly property var catalog: makeCatalog()
    readonly property var records: editing ? draft : Tiles.parse(layoutJson, catalog)
    readonly property var placements: Tiles.pack(records, catalog, columns)
    readonly property var selectedSpec: catalog.find(c => c.id === selectedId) || null
    readonly property var selectedRecord: records.find(t => t.id === selectedId) || null
    readonly property var selectedSizes: selectedSpec ? Tiles.sizes(selectedSpec.kind) : []
    readonly property var availableTiles: catalog.filter(c => !records.some(t => t.id === c.id))
    property string gestureId: ""
    property string gestureKind: ""
    property real pointerX: 0
    property real pointerY: 0
    property real pointerViewY: 0
    property string dropId: ""
    property bool dropAfter: false
    property var resizeCandidate: null
    readonly property var gesturePlacement: placements.find(p => p.id === gestureId) || null
    spacing: 8
    implicitWidth: 410
    implicitHeight: contentColumn.implicitHeight + actionIsland.height + 40

    function includes(plugin) { return customItems.includes(plugin); }
    function makeCatalog() {
        const result = [];
        function add(id, kind, title, plugin, w, h) {
            if (availableCustomItems.includes(plugin)) result.push({id:id,kind:kind,title:title,plugin:plugin,w:w,h:h,enabledByDefault:customItems.includes(plugin)});
        }
        add("wifi", "toggle", backend.wifiAvailable ? qsTr("Wi-Fi") : qsTr("Network"), "org.kde.plasma.networkmanagement", 3, 1);
        add("bluetooth", "toggle", qsTr("Bluetooth"), "org.kde.plasma.bluetooth", 3, 1);
        add("focus", "toggle", qsTr("Do Not Disturb"), "org.kde.plasma.notifications", 3, 1);
        add("microphone", "toggle", qsTr("Microphone"), "org.kde.plasma.volume", 3, 1);
        add("display", "display", qsTr("Display"), "org.kde.plasma.brightness", 6, 2);
        add("media", "media", qsTr("Media"), "org.kde.plasma.mediacontroller", 2, 2);
        add("volume", "volume", qsTr("Sound"), "org.kde.plasma.volume", 4, 1);
        add("output", "output", qsTr("Speakers"), "org.kde.plasma.volume", 4, 1);
        for (const entry of availableOtherItems) {
            const id = "tray:" + entry.id;
            if (!result.some(c => c.id === id)) result.push({id:id,kind:"tray",title:entry.title || entry.id,plugin:entry.id,entry:entry,w:3,h:1,enabledByDefault:otherItems.some(e => e.id === entry.id)});
        }
        return result;
    }
    function subtitle(id) {
        switch (id) {
        case "wifi": return backend.networkName;
        case "bluetooth": return backend.bluetoothAvailable ? backend.bluetoothName : qsTr("No adapter");
        case "focus": return backend.dndEnabled ? qsTr("Notifications paused") : qsTr("Notifications on");
        case "microphone": return !backend.microphoneAvailable ? qsTr("No microphone") : backend.microphoneMuted ? qsTr("Muted") : qsTr("Ready");
        case "output": return backend.outputName;
        }
        return "";
    }
    function symbol(id) {
        switch (id) {
        case "wifi": return backend.networkIcon;
        case "bluetooth": return "network-bluetooth-symbolic";
        case "focus": return backend.dndEnabled ? "notifications-disabled-symbolic" : "notifications-symbolic";
        case "microphone": return backend.microphoneMuted ? "microphone-sensitivity-muted-symbolic" : "audio-input-microphone-symbolic";
        case "output": return "audio-speakers-symbolic";
        }
        return "preferences-system";
    }
    function isSelected(id) {
        return id === "wifi" ? backend.wifiAvailable && backend.wifiEnabled
            : id === "bluetooth" ? backend.bluetoothEnabled
            : id === "focus" ? backend.dndEnabled : id === "microphone" && backend.microphoneMuted;
    }
    function isAvailable(id) {
        return id === "wifi" ? !backend.wifiAvailable || backend.wifiToggleAllowed
            : id === "bluetooth" ? backend.bluetoothAvailable : id === "focus" ? backend.dndAvailable
            : id === "microphone" ? backend.microphoneAvailable : true;
    }
    function activate(id) {
        if (editing || !isAvailable(id)) return;
        if (id === "wifi" && backend.wifiAvailable) backend.toggleWifi();
        else if (id === "bluetooth") backend.toggleBluetooth();
        else if (id === "focus") backend.toggleDnd();
        else if (id === "microphone") backend.toggleMicrophone();
        else { const spec = catalog.find(c => c.id === id); if (spec) openApplet(spec.plugin); }
    }
    function beginEdit() {
        draft = Tiles.parse(layoutJson, catalog);
        selectedId = placements.length ? placements[0].id : "";
        editing = true;
    }
    function endGesture() { gestureId = ""; gestureKind = ""; dropId = ""; resizeCandidate = null; }
    function cancelEdit() { endGesture(); editing = false; draft = []; }
    function saveEdit() {
        if (!editing) return;
        const json = Tiles.serialize(draft);
        endGesture();
        layoutCommitted(json);
        editing = false;
        // layoutJson stays bound to the owning tray's configuration.
    }
    function resetLayout() { if (editing) { draft = Tiles.defaults(catalog); selectedId = draft.length ? draft[0].id : ""; } }
    function addTile(id) {
        if (!editing || draft.some(t => t.id === id) || draft.length >= 64) return;
        const spec = catalog.find(c => c.id === id);
        if (spec) { draft = draft.concat([{id:id,w:spec.w,h:spec.h}]); selectedId = id; }
    }
    function removeTile(id) {
        if (!editing) return;
        draft = draft.filter(t => t.id !== id);
        if (selectedId === id) selectedId = placements.length ? placements[0].id : "";
    }
    function setSize(id, w, h) {
        if (!editing) return;
        const spec = catalog.find(c => c.id === id);
        if (!spec) return;
        const size = Tiles.nearest(spec.kind, w, h);
        draft = draft.map(t => t.id === id ? {id:id,w:size.w,h:size.h} : t);
    }
    function cycleSize(id) {
        const spec = catalog.find(c => c.id === id), record = records.find(t => t.id === id);
        if (!spec || !record) return;
        const sizes = Tiles.sizes(spec.kind);
        const index = sizes.findIndex(s => s.w === record.w && s.h === record.h);
        const size = sizes[(index + 1) % sizes.length];
        setSize(id, size.w, size.h);
    }
    function moveTile(id, target, after) { if (editing) draft = Tiles.move(draft, id, target, after); }
    function moveSelected(offset) {
        const index = placements.findIndex(p => p.id === selectedId);
        if (index >= 0 && index + offset >= 0 && index + offset < placements.length)
            moveTile(selectedId, placements[index + offset].id, offset > 0);
    }
    function updateDrag(id, x, y) {
        gestureId = id; gestureKind = "move"; pointerX = x; pointerY = y;
        pointerViewY = y - scroller.contentY;
        findDropTarget();
    }
    function findDropTarget() {
        let nearest = null, distance = Infinity;
        for (const p of placements) {
            if (p.id === gestureId) continue;
            const cx = p.col * (unit + gridGap) + (p.w * (unit + gridGap) - gridGap) / 2;
            const cy = p.row * (unit + gridGap) + (p.h * (unit + gridGap) - gridGap) / 2;
            const d = Math.pow(pointerX - cx, 2) + Math.pow(pointerY - cy, 2);
            if (d < distance) { distance = d; nearest = p; }
        }
        dropId = nearest ? nearest.id : "";
        dropAfter = !!nearest && pointerY > nearest.row * (unit + gridGap) + (nearest.h * (unit + gridGap) - gridGap) * 0.65;
    }
    function finishDrag() {
        const id = gestureId, target = dropId, after = dropAfter;
        endGesture();
        if (id && target) moveTile(id, target, after);
    }
    function updateResize(id, dx, dy) {
        const record = records.find(t => t.id === id), spec = catalog.find(c => c.id === id);
        if (!record || !spec) return;
        gestureId = id; gestureKind = "resize";
        resizeCandidate = Tiles.nearest(spec.kind, record.w + dx / (unit + gridGap), record.h + dy / (unit + gridGap));
    }
    function finishResize() {
        const id = gestureId, size = resizeCandidate;
        endGesture();
        if (id && size) setSize(id, size.w, size.h);
    }
    onVisibleChanged: if (!visible && editing) cancelEdit()
    Keys.onEscapePressed: event => { if (editing) { cancelEdit(); event.accepted = true; } else event.accepted = false; }

    component IslandButton: Popup.GlassToolButton {
        id: action
        contentItem: RowLayout {
            spacing: 6
            Kirigami.Icon {
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                source: action.icon.name
                isMask: true
                color: action.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            }
            PC.Label {
                visible: action.display !== PC.AbstractButton.IconOnly
                text: action.text
                font: action.font
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: dashboard.editing
        Popup.GlassToolButton { objectName: "moveTileEarlier"; text: qsTr("Move earlier"); icon.name: "go-previous"; display: PC.AbstractButton.IconOnly; enabled: !!dashboard.selectedSpec; onClicked: dashboard.moveSelected(-1); PC.ToolTip { text: parent.text } }
        Popup.GlassToolButton { objectName: "moveTileLater"; text: qsTr("Move later"); icon.name: "go-next"; display: PC.AbstractButton.IconOnly; enabled: !!dashboard.selectedSpec; onClicked: dashboard.moveSelected(1); PC.ToolTip { text: parent.text } }
        PC.ComboBox {
            id: sequoiaMenuSelector0
            property QtObject sequoiaSelectorLifetime: Details.DetailPopupLifetime {
                popup: sequoiaMenuSelector0.popup
                owner: sequoiaMenuSelector0
            }
            objectName: "tileSizePicker"
            implicitWidth: 120
            implicitHeight: Math.max(32, Kirigami.Theme.defaultFont.pointSize * 2 + 12)
            Layout.fillWidth: true
            enabled: !!dashboard.selectedSpec
            model: dashboard.selectedSizes.map(s => s.w + " × " + s.h)
            currentIndex: dashboard.selectedRecord ? dashboard.selectedSizes.findIndex(s => s.w === dashboard.selectedRecord.w && s.h === dashboard.selectedRecord.h) : -1
            Accessible.name: qsTr("Tile size")
            onActivated: index => { const size = dashboard.selectedSizes[index]; if (size) dashboard.setSize(dashboard.selectedId, size.w, size.h); }
        }
        Popup.GlassToolButton { objectName: "removeSelectedTile"; text: qsTr("Remove tile"); icon.name: "list-remove"; display: PC.AbstractButton.IconOnly; enabled: !!dashboard.selectedSpec; onClicked: dashboard.removeTile(dashboard.selectedId); PC.ToolTip { text: parent.text } }
    }
    PC.Label {
        Layout.fillWidth: true
        visible: dashboard.editing
        text: qsTr("Drag to move · drag the corner to resize")
        wrapMode: Text.WordWrap
        color: Kirigami.Theme.disabledTextColor
        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9
    }
    Item {
        id: viewport
        Layout.fillWidth: true
        Layout.fillHeight: true
        PC.ScrollView {
            id: scroll
            objectName: "quickSettingsScroll"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: actionIsland.top
            anchors.bottomMargin: -8
            layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: edgeMask
                maskThresholdMin: 0.5
                maskSpreadAtMin: 1.0
                autoPaddingEnabled: false
            }
            background: null
            PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
            PC.ScrollBar.vertical.policy: dashboard.editing ? PC.ScrollBar.AlwaysOn : PC.ScrollBar.AlwaysOff
            contentItem: Flickable {
                id: scroller
                objectName: "quickSettingsFlickable"
                contentWidth: width
                contentHeight: contentColumn.height + 32
                boundsBehavior: Flickable.StopAtBounds
                interactive: dashboard.gestureKind === ""
                clip: true
                ColumnLayout {
                    id: contentColumn
                    y: 4
                    width: dashboard.gridWidth
                    x: (scroller.width - width) / 2
                    spacing: dashboard.gridGap
                    Item {
                        id: tileGrid
                        objectName: "quickSettingsGrid"
                        Layout.fillWidth: true
                        implicitHeight: dashboard.placements.reduce((max, p) => Math.max(max, (p.row + p.h) * (dashboard.unit + dashboard.gridGap) - dashboard.gridGap), 0)
                        Repeater {
                            id: tileRepeater
                            model: dashboard.placements
                            delegate: QuickSettingsFrame {
                                required property var modelData
                                controller: dashboard
                                placement: modelData
                                grid: tileGrid
                                x: modelData.col * (dashboard.unit + dashboard.gridGap)
                                y: modelData.row * (dashboard.unit + dashboard.gridGap)
                                width: modelData.w * (dashboard.unit + dashboard.gridGap) - dashboard.gridGap
                                height: modelData.h * (dashboard.unit + dashboard.gridGap) - dashboard.gridGap
                            }
                        }
                        Rectangle {
                            visible: !!dashboard.gesturePlacement
                            z: 100
                            x: dashboard.gestureKind === "move" ? dashboard.pointerX - width / 2 : (dashboard.gesturePlacement?.col ?? 0) * (dashboard.unit + dashboard.gridGap)
                            y: dashboard.gestureKind === "move" ? dashboard.pointerY - 24 : (dashboard.gesturePlacement?.row ?? 0) * (dashboard.unit + dashboard.gridGap)
                            width: ((dashboard.gestureKind === "resize" ? Math.min(dashboard.columns, dashboard.resizeCandidate?.w ?? 1) : dashboard.gesturePlacement?.w ?? 1) * (dashboard.unit + dashboard.gridGap)) - dashboard.gridGap
                            height: ((dashboard.gestureKind === "resize" ? dashboard.resizeCandidate?.h ?? 1 : dashboard.gesturePlacement?.h ?? 1) * (dashboard.unit + dashboard.gridGap)) - dashboard.gridGap
                            radius: 20
                            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.2)
                            border.color: Kirigami.Theme.highlightColor
                            border.width: 2
                            PC.Label { anchors.centerIn: parent; width: parent.width - 16; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight; text: dashboard.gesturePlacement?.spec.title ?? "" }
                        }
                    }
                    PC.Label {
                        Layout.fillWidth: true
                        visible: dashboard.placements.length === 0
                        text: dashboard.editing ? qsTr("Add tiles below to build your layout.") : qsTr("No tiles. Choose Edit tiles to add controls.")
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.disabledTextColor
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: dashboard.editing
                        spacing: 8
                        PC.Label { text: qsTr("Add tiles"); font.weight: Font.DemiBold }
                        Repeater {
                            model: dashboard.availableTiles
                            delegate: Popup.GlassButton {
                                required property var modelData
                                objectName: "addTile:" + modelData.id
                                Layout.fillWidth: true
                                enabled: dashboard.draft.length < 64
                                text: qsTr("Add %1").arg(modelData.title)
                                icon.name: "list-add"
                                onClicked: dashboard.addTile(modelData.id)
                            }
                        }
                        PC.Label { Layout.fillWidth: true; visible: dashboard.availableTiles.length === 0; text: qsTr("All available tiles are already added."); wrapMode: Text.WordWrap; color: Kirigami.Theme.disabledTextColor }
                        PC.Label { Layout.fillWidth: true; text: qsTr("Enable more applets in System Tray settings to make them available here."); wrapMode: Text.WordWrap; color: Kirigami.Theme.disabledTextColor; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9 }
                        Popup.GlassButton { objectName: "resetTileLayout"; text: qsTr("Reset layout"); onClicked: dashboard.resetLayout() }
                    }
                }
            }
        }

        // A real alpha mask keeps the popup's glass visible through both edges.
        Rectangle {
            id: edgeMask
            objectName: "quickSettingsEdgeMask"
            width: scroll.width
            height: scroll.height
            visible: false
            layer.enabled: true
            readonly property real fade: Math.min(24, height / 3) / Math.max(1, height)
            gradient: Gradient {
                GradientStop { position: 0; color: scroller.contentY > 1 ? "transparent" : "white" }
                GradientStop { position: edgeMask.fade; color: "white" }
                GradientStop { position: 1 - edgeMask.fade; color: "white" }
                GradientStop { position: 1; color: "transparent" }
            }
        }
        ControlGlassSurface {
            id: actionIsland
            objectName: "quickSettingsActionIsland"
            z: 10
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6
            width: Math.min(parent.width - 8, islandActions.implicitWidth + 24)
            height: islandActions.implicitHeight + 12
            radius: height / 2
            color: Qt.alpha(stateColor, dark ? 0.94 : 0.90)
            shadow.size: 14
            shadow.yOffset: 3
            shadow.color: Qt.rgba(0, 0, 0, dark ? 0.30 : 0.16)
            // Do not pass clicks in the island's padding to controls behind it.
            MouseArea { anchors.fill: parent }
            RowLayout {
                id: islandActions
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                spacing: 4
                IslandButton {
                    objectName: "editTilesButton"
                    visible: !dashboard.editing
                    text: qsTr("Edit tiles")
                    icon.name: "document-edit"
                    icon.color: Kirigami.Theme.textColor
                    display: viewport.width < 320 ? PC.AbstractButton.IconOnly : PC.AbstractButton.TextBesideIcon
                    onClicked: dashboard.beginEdit()
                    PC.ToolTip { text: parent.text }
                }
                Popup.GlassButton { objectName: "cancelTileEditing"; visible: dashboard.editing; text: qsTr("Cancel"); onClicked: dashboard.cancelEdit() }
                Popup.GlassButton { objectName: "saveTileEditing"; visible: dashboard.editing; text: qsTr("Done"); onClicked: dashboard.saveEdit() }
                Rectangle {
                    visible: !dashboard.editing
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 20
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.16)
                }
                IslandButton { objectName: "islandSettingsButton"; visible: !dashboard.editing; text: qsTr("System Settings"); icon.name: "configure"; icon.color: Kirigami.Theme.textColor; display: PC.AbstractButton.IconOnly; onClicked: dashboard.backend.settings(); PC.ToolTip { text: parent.text } }
                IslandButton { objectName: "islandLockButton"; visible: !dashboard.editing; text: qsTr("Lock screen"); icon.name: "system-lock-screen-symbolic"; icon.color: Kirigami.Theme.textColor; display: PC.AbstractButton.IconOnly; enabled: dashboard.backend.canLock; onClicked: dashboard.backend.lock(); PC.ToolTip { text: parent.text } }
                IslandButton { objectName: "islandPowerButton"; visible: !dashboard.editing; text: qsTr("Power and session"); icon.name: "system-shutdown-symbolic"; icon.color: Kirigami.Theme.textColor; display: PC.AbstractButton.IconOnly; onClicked: dashboard.backend.powerOptions(); PC.ToolTip { text: parent.text } }
            }
        }
    }
    Timer {
        interval: 40
        repeat: true
        running: dashboard.gestureKind === "move"
        onTriggered: {
            const delta = dashboard.pointerViewY < 40 ? -12 : dashboard.pointerViewY > scroller.height - 40 ? 12 : 0;
            const before = scroller.contentY;
            scroller.contentY = Math.max(0, Math.min(Math.max(0, scroller.contentHeight - scroller.height), before + delta));
            dashboard.pointerY += scroller.contentY - before;
            dashboard.findDropTarget();
        }
    }
}
