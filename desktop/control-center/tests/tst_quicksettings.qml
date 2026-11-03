// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Dashboard
import "../QuickSettingsLayout.js" as Tiles

Item {
    id: stage
    width: 442; height: 800
    Dashboard.Preview { id: preview; anchors.fill: parent }
    Component { id: restoredPreview; Dashboard.Preview {} }
    TestCase {
        name: "QuickSettings"
        when: windowShown
        function findVisual(root, name) {
            if (root.objectName === name) return root;
            for (const child of root.children || []) {
                const found = findVisual(child, name);
                if (found) return found;
            }
            return null;
        }
        function find(name) { const item = findVisual(preview, name); verify(item !== null, "Missing " + name); return item; }
        function reveal(item) {
            const scroll = find("quickSettingsFlickable");
            let ancestor = item.parent;
            while (ancestor && ancestor !== scroll.contentItem) ancestor = ancestor.parent;
            if (!ancestor) return; // Fixed island actions must not scroll the grid.
            const point = item.mapToItem(scroll.contentItem, 0, 0);
            scroll.contentY = Math.max(0, Math.min(scroll.contentHeight - scroll.height, point.y - 30));
            wait(40);
        }
        function click(name) { const item = find(name); reveal(item); mouseClick(item); wait(30); }
        function reset() {
            preview.dashboard.cancelEdit();
            preview.dashboard.availableCustomItems = Qt.binding(() => preview.dashboard.customItems);
            preview.dashboard.availableOtherItems = Qt.binding(() => preview.dashboard.otherItems);
            stage.width = 442; stage.height = 800;
            preview.savedLayout = "";
            preview.requestedApplet = "";
            preview.fixture.wifiEnabled = true;
            preview.fixture.bluetoothEnabled = true;
            preview.fixture.dndEnabled = false;
            preview.fixture.microphoneMuted = false;
            preview.fixture.volume = 64; preview.fixture.muted = false; preview.fixture.playing = true;
            preview.dashboard.otherItems = [{id:"org.kde.merkuro.contact.applet",title:"Contacts",subtitle:"Contact list",icon:"user-identity",kind:"Plasmoid",applet:null}];
            wait(60);
            find("quickSettingsFlickable").contentY = 0;
        }
        function init() { reset(); }
        function cleanup() { preview.dashboard.cancelEdit(); }
        function record(id) { return preview.dashboard.records.find(t => t.id === id); }
        function verifyGeometry() {
            const placed = preview.dashboard.placements, grid = find("quickSettingsGrid");
            for (let i = 0; i < placed.length; ++i) {
                const a = find("tileFrame:" + placed[i].id);
                verify(a.width > 40 && a.height >= 50, "usable tile dimensions");
                verify(a.x >= -0.1 && a.x + a.width <= grid.width + 0.1, "tile fits width: " + placed[i].id + " " + a.x + "+" + a.width + " <= " + grid.width);
                for (let j = i + 1; j < placed.length; ++j) {
                    const b = find("tileFrame:" + placed[j].id);
                    verify(a.x + a.width <= b.x + 0.1 || b.x + b.width <= a.x + 0.1 || a.y + a.height <= b.y + 0.1 || b.y + b.height <= a.y + 0.1, "no overlap");
                }
            }
        }
        function test_controls_and_long_press() {
            for (const pair of [["Wi-FiToggle","wifiEnabled"],["BluetoothToggle","bluetoothEnabled"],["Do Not DisturbToggle","dndEnabled"],["MicrophoneToggle","microphoneMuted"],["muteButton","muted"],["playPauseButton","playing"]]) {
                const before = preview.fixture[pair[1]];
                click(pair[0]);
                compare(preview.fixture[pair[1]], !before);
            }
            const volume = find("volumeSlider"); volume.forceActiveFocus();
            keyClick(Qt.Key_Right); compare(preview.fixture.volume, 65);
            preview.fixture.volume = 31; wait(20); compare(volume.value, 31);
            const brightness = find("brightnessSlider:display-1"); brightness.forceActiveFocus();
            keyClick(Qt.Key_Right); compare(preview.fixture.displays.get(0).brightness, 73);
            const wifi = find("Wi-FiToggle"); reveal(wifi);
            const before = preview.fixture.wifiEnabled;
            mousePress(wifi); wait(1000); mouseRelease(wifi); wait(30);
            compare(preview.requestedApplet, "org.kde.plasma.networkmanagement");
            compare(preview.fixture.wifiEnabled, before, "hold must not also toggle");
        }
        function test_fixed_island_actions_and_scroll_clearance() {
            stage.height = 500;
            const island = find("quickSettingsActionIsland"), scroll = find("quickSettingsFlickable");
            wait(60);
            const original = island.mapToItem(preview, 0, 0);
            verify(scroll.contentHeight > scroll.height);
            for (const position of [0, (scroll.contentHeight - scroll.height) / 2, scroll.contentHeight - scroll.height]) {
                scroll.contentY = position; wait(30);
                compare(island.mapToItem(preview, 0, 0), original, "island stays fixed while scrolling");
                for (const action of [["islandSettingsButton", "settings"], ["islandLockButton", "lock"], ["islandPowerButton", "power"]]) {
                    preview.fixture.lastSessionAction = "";
                    mouseClick(find(action[0]));
                    compare(preview.fixture.lastSessionAction, action[1]);
                    compare(scroll.contentY, position);
                }
            }
            const lastTile = find("tileFrame:tray:org.kde.merkuro.contact.applet");
            const bottom = lastTile.mapToItem(scroll, 0, lastTile.height).y;
            verify(bottom <= scroll.height - 24, "last tile can clear the edge fade and island");
            mouseClick(find("editTilesButton")); wait(40);
            verify(preview.dashboard.editing);
            scroll.contentY = scroll.contentHeight - scroll.height;
            mouseClick(find("cancelTileEditing")); wait(40);
            verify(!preview.dashboard.editing, "editing can be cancelled from the bottom of the list");
            stage.width = 292; wait(40);
            verify(island.width <= preview.dashboard.width);
            for (const name of ["editTilesButton", "islandSettingsButton", "islandLockButton", "islandPowerButton"]) {
                const button = find(name), point = button.mapToItem(island, 0, 0);
                verify(point.x >= 0 && point.x + button.width <= island.width, "island action fits narrow popup");
            }
        }
        function test_edit_remove_add_save_restore_cancel() {
            click("editTilesButton"); verify(preview.dashboard.editing);
            const before = preview.fixture.wifiEnabled;
            click("moveHandle:wifi"); compare(preview.fixture.wifiEnabled, before);
            click("removeTile:wifi"); verify(!record("wifi"));
            click("addTile:wifi"); verify(!!record("wifi"));
            preview.dashboard.addTile("wifi"); compare(preview.dashboard.records.filter(t => t.id === "wifi").length, 1);
            preview.dashboard.setSize("wifi", 1, 1);
            preview.dashboard.moveTile("wifi", "bluetooth", false);
            click("saveTileEditing"); verify(!preview.dashboard.editing);
            const saved = preview.savedLayout;
            compare(JSON.parse(saved).version, 1); compare(record("wifi").w, 1);
            const restored = createTemporaryObject(restoredPreview, stage, {savedLayout:saved,visible:false});
            verify(restored); compare(restored.dashboard.records[0].id, "wifi");
            compare(restored.dashboard.records.find(t => t.id === "wifi").w, 1);
            click("editTilesButton"); preview.dashboard.removeTile("wifi"); click("cancelTileEditing");
            compare(preview.savedLayout, saved); verify(!!record("wifi"));
            click("editTilesButton"); preview.dashboard.resetLayout(); click("cancelTileEditing");
            compare(preview.savedLayout, saved);
            click("editTilesButton"); preview.dashboard.resetLayout(); click("saveTileEditing");
            compare(record("wifi").w, 3);
        }
        function test_drag_move_and_resize_handles() {
            click("editTilesButton");
            const mover = find("moveHandle:bluetooth"); reveal(mover);
            mousePress(mover, 25, 30);
            mouseMove(mover, -100, 30, 60);
            compare(preview.dashboard.gestureKind, "move");
            mouseRelease(mover, -100, 30); wait(50);
            compare(preview.dashboard.records[0].id, "bluetooth");
            const resize = find("resizeHandle:wifi"); reveal(resize);
            mousePress(resize, 15, 12);
            mouseMove(resize, -140, 12, 60);
            compare(preview.dashboard.gestureKind, "resize");
            mouseRelease(resize, -140, 12); wait(50);
            compare(record("wifi").w, 1); compare(record("wifi").h, 1);
            verifyGeometry();
            preview.dashboard.setSize("media", 1, 1);
            compare(record("media").w, 2); compare(record("media").h, 2);
            preview.dashboard.setSize("display", 99, 99);
            compare(record("display").w, 6); compare(record("display").h, 3);
        }
        function test_every_size_and_narrow_layout() {
            click("editTilesButton");
            for (const width of [292, 442, 552]) {
                stage.width = width; wait(50);
                for (const spec of preview.dashboard.catalog) {
                    for (const size of Tiles.sizes(spec.kind)) {
                        preview.dashboard.setSize(spec.id, size.w, size.h); wait(30);
                        verifyGeometry();
                    }
                }
            }
        }
        function test_empty_missing_apps_and_corrupt_settings() {
            preview.savedLayout = '{"version":1,"tiles":[]}'; wait(20);
            compare(preview.dashboard.placements.length, 0);
            click("editTilesButton"); click("addTile:wifi"); click("saveTileEditing");
            compare(preview.dashboard.placements.length, 1);
            preview.savedLayout = "broken json"; wait(30); verify(preview.dashboard.placements.length > 0);
            click("editTilesButton"); click("saveTileEditing");
            const saved = preview.savedLayout;
            preview.dashboard.otherItems = []; wait(20);
            verify(record("tray:org.kde.merkuro.contact.applet"));
            verify(!preview.dashboard.placements.some(p => p.id === "tray:org.kde.merkuro.contact.applet"));
            compare(preview.savedLayout, saved);
            preview.dashboard.otherItems = [{id:"org.kde.merkuro.contact.applet",title:"Contacts",subtitle:"Contact list",icon:"user-identity",kind:"Plasmoid",applet:null}];
            wait(20); verify(preview.dashboard.placements.some(p => p.id === "tray:org.kde.merkuro.contact.applet"));
        }
        function test_add_panel_only_entries_and_sanitize() {
            const entries = preview.dashboard.otherItems;
            preview.dashboard.availableOtherItems = entries;
            preview.dashboard.otherItems = [];
            wait(20);
            verify(!record("tray:org.kde.merkuro.contact.applet"));
            click("editTilesButton"); click("addTile:tray:org.kde.merkuro.contact.applet"); click("saveTileEditing");
            verify(record("tray:org.kde.merkuro.contact.applet"));
            const catalog = preview.dashboard.catalog;
            const decoded = Tiles.parse(JSON.stringify({version:1,tiles:[{id:"wifi",w:1000,h:200},{id:"wifi",w:1,h:1},{id:"media",w:-1,h:0},{id:"display",w:1,h:1},null,{id:1}]}),catalog);
            compare(decoded.length,3);
            compare(decoded[0].w,3);compare(decoded[0].h,1);
            compare(decoded[1].w,2);compare(decoded[1].h,2);
            compare(decoded[2].w,3);compare(decoded[2].h,2);
        }
        function test_keyboard_editing_and_no_settings_leak_on_close() {
            click("editTilesButton");
            const frame = find("tileFrame:wifi"); reveal(frame); frame.forceActiveFocus();
            keyClick(Qt.Key_Space); wait(30); compare(record("wifi").w, 1);
            click("moveTileLater"); compare(preview.dashboard.records[1].id, "wifi");
            preview.dashboard.visible = false; wait(20); verify(!preview.dashboard.editing);
            preview.dashboard.visible = true; compare(preview.savedLayout, ""); compare(record("wifi").w, 3);
        }
    }
}
