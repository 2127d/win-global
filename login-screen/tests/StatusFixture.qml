// SPDX-License-Identifier: MIT
// Simulated hardware for isolated UI tests and explicitly requested demo previews.
import QtQuick
QtObject {
    property string networkIcon: "network-wireless-100-symbolic"
    property bool wifiAvailable: true
    property bool wifiEnabled: true
    property bool wifiCanChange: true
    property bool wifiPending: false
    property string networkSummary: "Forest Wi-Fi"
    property string wifiHint: wifiEnabled ? "Wi-Fi is on" : "Wi-Fi is off"
    property string networkError: ""
    property bool audioAvailable: true
    property bool muted: false
    property int volumePercent: 64
    property string audioDescription: "Built-in speakers"
    property string audioError: ""
    property bool hasBattery: true
    property int batteryPercent: 76
    property bool pluggedIn: false
    property string batteryIcon: "battery-80-symbolic"
    property string batterySummary: "On battery power"
    property string batteryRemaining: "About 4 h 20 min remaining"
    property int wifiRequests: 0
    property int audioRequests: 0
    function setWifiEnabled(value) { wifiRequests++; wifiEnabled = value; }
    function setVolume(value) { audioRequests++; volumePercent = Math.round(value); }
    function toggleMute() { audioRequests++; muted = !muted; }
}
