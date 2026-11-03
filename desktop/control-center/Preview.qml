// SPDX-License-Identifier: MIT
// Deterministic rendering/interaction fixture; never installed in the tray.
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

Rectangle {
    id: preview
    width: 442; height: 650
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property bool live: false
    property string requestedApplet: ""
    property string savedLayout: ""
    property alias dashboard: dashboard
    Loader { id: liveBackend; active: preview.live; source: "ControlBackend.qml" }
    QtObject {
        id: fixture
        property bool wifiAvailable: true
        property bool wifiEnabled: true
        property bool wifiToggleAllowed: true
        property string networkIcon: "network-wireless-symbolic"
        property string networkName: wifiEnabled ? "Home network" : "Off"
        property bool bluetoothAvailable: true
        property bool bluetoothEnabled: true
        property string bluetoothName: bluetoothEnabled ? "Headphones connected" : "Off"
        property bool dndAvailable: true
        property bool dndEnabled: false
        property bool volumeAvailable: true
        property bool muted: false
        property int volume: 64
        property string outputName: "Built-in Audio · Analog Stereo"
        property bool microphoneAvailable: true
        property bool microphoneMuted: true
        property bool brightnessAvailable: true
        property var displays: ListModel {
            ListElement { displayName: "display-1"; label: "Main display"; brightness: 72; maxBrightness: 100 }
        }
        property string track: "A little room to focus"
        property string artist: "Now playing · Preview fixture"
        property string artwork: ""
        property bool playing: true
        property bool canPlayPause: true
        property bool canPrevious: true
        property bool canNext: true
        property bool canLock: true
        property string lastSessionAction: ""
        function toggleWifi() { wifiEnabled = !wifiEnabled; }
        function toggleBluetooth() { bluetoothEnabled = !bluetoothEnabled; }
        function toggleDnd() { dndEnabled = !dndEnabled; }
        function toggleMicrophone() { microphoneMuted = !microphoneMuted; }
        function toggleMute() { muted = !muted; }
        function setVolume(value) { volume = Math.round(value); }
        function setBrightness(display, value) { displays.setProperty(0, "brightness", Math.round(value)); }
        function playPause() { playing = !playing; }
        function previous() {}
        function next() {}
        function settings() { lastSessionAction = "settings"; }
        function lock() { lastSessionAction = "lock"; }
        function powerOptions() { lastSessionAction = "power"; }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12
        spacing: 12
        ControlGlassSurface {
            Layout.fillWidth: true
            implicitHeight: headerRow.implicitHeight + 8
            radius: height / 2
            elevated: true
            RowLayout {
                id: headerRow
                anchors.fill: parent
                anchors.margins: 4
                PC.Label { text: qsTr("Control Center"); font.pointSize: 12; font.weight: Font.DemiBold; Layout.fillWidth: true; Layout.leftMargin: 10 }
                ControlHeaderButton { icon.name: "window-pin-symbolic"; text: qsTr("Pin"); display: PC.AbstractButton.IconOnly }
                ControlHeaderButton { icon.name: "configure-symbolic"; text: qsTr("Configure"); display: PC.AbstractButton.IconOnly }
            }
        }
        ControlDashboard {
            id: dashboard
            objectName: "dashboardViewport"
            Layout.margins: 4
            Layout.fillWidth: true; Layout.fillHeight: true
            layoutJson: preview.savedLayout
            onLayoutCommitted: json => preview.savedLayout = json
            backend: preview.live && liveBackend.item ? liveBackend.item : fixture
            otherItems: preview.live ? [] : [{
                id: "org.kde.merkuro.contact.applet", title: "Contacts", subtitle: "Contact list",
                icon: "user-identity", kind: "Plasmoid", applet: null
            }]
            onOpenApplet: plugin => preview.requestedApplet = plugin
        }
    }
}
