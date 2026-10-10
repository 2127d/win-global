// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import "../qml" as Theme

Item {
    id: scene
    width: 1280; height: 720
    property real unit: Math.max(0.65, Math.min(2, width / 1024, height / 560))
    property real loginUnit: Math.max(0.75, unit * 0.8)
    property url wallpaper: Qt.resolvedUrl("backdrop.svg")
    property string clockText: "10:09"
    property string dateText: "Wednesday, October 16"
    StatusFixture { id: state }
    ListModel {
        id: sessions
        ListElement { display: "Plasma" }
        ListElement { display: "Plasma Bigscreen" }
    }
    Theme.StatusIsland {
        id: island
        scene: scene; status: state
        anchors.right: parent.right; anchors.top: parent.top
        anchors.rightMargin: 18 * scene.unit; anchors.topMargin: 8 * scene.unit
        width: implicitWidth; height: implicitHeight
        property int selectionRequests: 0
        onSessionSelected: index => { sessionIndex = index; selectionRequests++; }
    }
    TestCase {
        name: "StatusIsland"
        when: windowShown
        function visualChild(item, name) {
            if (item.objectName === name) return item;
            for (const child of item.children || []) {
                const found = visualChild(child, name);
                if (found) return found;
            }
            return null;
        }
        function button(name) {
            // ScrollView reparents its delegates into its internal content item.
            return findChild(island, name) || visualChild(island.popup.contentItem, name);
        }
        function init() {
            island.popup.close(); island.enabled = true;
            island.sessionModel = sessions; island.sessionIndex = 0; island.selectionRequests = 0;
            scene.width = 1280; scene.height = 720;
            state.hasBattery = true; state.audioAvailable = true;
            state.wifiEnabled = true; state.wifiCanChange = true; state.wifiPending = false;
            state.wifiRequests = 0; state.audioRequests = 0; state.muted = false; state.volumePercent = 64;
            wait(30);
        }
        function test_sections_and_escape() {
            for (const entry of [["networkStatusButton", "network"], ["audioStatusButton", "audio"], ["batteryStatusButton", "battery"], ["clockStatusButton", "clock"]]) {
                mouseClick(button(entry[0]));
                tryCompare(island.popup, "opened", true);
                compare(island.section, entry[1]);
                keyClick(Qt.Key_Escape);
                tryCompare(island.popup, "opened", false);
            }
        }
        function verifyCentered(name) {
            const action = button(name);
            const contents = button(name + "Contents");
            let left = Infinity, right = -Infinity, top = Infinity, bottom = -Infinity;
            for (const child of contents.children) {
                if (!child.visible) continue;
                const p = child.mapToItem(action, 0, 0);
                left = Math.min(left, p.x); right = Math.max(right, p.x + child.width);
                top = Math.min(top, p.y); bottom = Math.max(bottom, p.y + child.height);
            }
            fuzzyCompare((left + right) / 2, action.width / 2, 0.5, name + " horizontal centering");
            fuzzyCompare((top + bottom) / 2, action.height / 2, 0.5, name + " vertical centering");
        }
        function test_active_and_hover_centering() {
            for (const entry of [["networkStatusButton", "network"], ["audioStatusButton", "audio"], ["clockStatusButton", "clock"]]) {
                island.showSection(entry[1]);
                tryCompare(island.popup, "opened", true);
                waitForRendering(island.popup.contentItem);
                verifyCentered(entry[0]);
                const close = button("statusCloseButton");
                mouseMove(close, close.width / 2, close.height / 2);
                tryCompare(close, "hovered", true);
                verifyCentered("statusCloseButton");
                island.popup.close();
                tryCompare(island.popup, "opened", false);
            }
        }
        function test_network_controls_and_permission() {
            mouseClick(button("networkStatusButton"));
            tryCompare(island.popup, "opened", true);
            waitForRendering(island.popup.contentItem);
            mouseClick(button("wifiToggle"));
            compare(state.wifiEnabled, false); compare(state.wifiRequests, 1);
            state.wifiCanChange = false;
            verify(!button("wifiToggle").enabled);
            mouseClick(button("wifiToggle")); compare(state.wifiRequests, 1);
        }
        function test_volume_mute_and_keyboard() {
            mouseClick(button("audioStatusButton"));
            tryCompare(island.popup, "opened", true);
            waitForRendering(island.popup.contentItem);
            mouseClick(button("muteToggle")); compare(state.muted, true);
            const slider = button("statusVolumeSlider");
            slider.forceActiveFocus(); keyClick(Qt.Key_Right);
            compare(state.volumePercent, 65); compare(state.audioRequests, 2);
            state.volumePercent = 30; compare(slider.value, 30);
            state.audioAvailable = false;
            verify(!slider.enabled); verify(!button("muteToggle").enabled);
        }
        function test_session_selection_and_keyboard() {
            // Switching panels must move keyboard focus into the selected session.
            island.showSection("audio");
            tryCompare(island.popup, "opened", true);
            mouseClick(button("sessionStatusButton"));
            tryCompare(island.popup, "opened", true);
            tryCompare(button("sessionChoice0"), "activeFocus", true);
            keyClick(Qt.Key_Down);
            tryCompare(button("sessionChoice1"), "activeFocus", true);
            keyClick(Qt.Key_Space);
            compare(island.sessionIndex, 1);
            compare(island.selectionRequests, 1);
            tryCompare(island.popup, "opened", false);
            mouseClick(button("sessionStatusButton"));
            tryCompare(island.popup, "opened", true);
            tryCompare(button("sessionChoice1"), "activeFocus", true);
            keyClick(Qt.Key_Escape);
            tryCompare(island.popup, "opened", false);
            compare(island.selectionRequests, 1);
        }
        function test_missing_battery_and_busy() {
            state.hasBattery = false;
            verify(!button("batteryStatusButton").visible);
            mouseClick(button("clockStatusButton"));
            tryCompare(island.popup, "opened", true);
            island.enabled = false;
            tryCompare(island.popup, "opened", false);
        }
        function test_popup_bounds_data() {
            return [{tag: "small", w: 800, h: 480}, {tag: "standard", w: 1280, h: 720}, {tag: "4k", w: 3840, h: 2160}];
        }
        function test_popup_bounds(data) {
            scene.width = data.w; scene.height = data.h;
            for (const section of ["audio", "session"]) {
                island.showSection(section);
                tryCompare(island.popup, "opened", true);
                verify(island.x >= 0);
                verify(island.x + island.width <= scene.width);
                verify(island.x + island.popup.x >= 0);
                verify(island.y + island.popup.y >= island.y + island.height);
                verify(island.y + island.popup.y + island.popup.height <= scene.height);
                island.popup.close();
                tryCompare(island.popup, "opened", false);
            }
        }
    }
}
