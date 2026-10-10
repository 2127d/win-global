// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.networkmanagement as NM
import org.kde.plasma.private.volume as PA
import org.kde.plasma.private.battery as Battery

Item {
    id: status
    visible: false
    NM.ConnectionIcon { id: network }
    NM.NetworkStatus { id: networkStatus }
    NM.AvailableDevices { id: devices }
    NM.EnabledConnections { id: connections }
    NM.Handler { id: networkHandler }
    Battery.BatteryControlModel { id: battery }

    readonly property string networkIcon: network.connectionIcon.length === 0 ? "network-offline-symbolic" :
        network.connectionIcon.endsWith("-symbolic") ? network.connectionIcon : network.connectionIcon + "-symbolic"
    readonly property bool wifiAvailable: devices.wirelessDeviceAvailable
    readonly property bool wifiEnabled: connections.wirelessEnabled
    readonly property bool wifiCanChange: wifiAvailable && connections.wirelessHwEnabled && connections.networkingEnabled
    readonly property string networkSummary: network.connecting ? qsTr("Connecting…") :
        networkStatus.activeConnections || qsTr("No active connection")
    readonly property string wifiHint: !wifiAvailable ? qsTr("No Wi-Fi adapter") :
        !connections.wirelessHwEnabled ? qsTr("Wi-Fi is blocked by the hardware switch") :
        !connections.networkingEnabled ? qsTr("Networking is disabled") :
        wifiEnabled ? qsTr("Wi-Fi is on") : qsTr("Wi-Fi is off")
    property string networkError: ""
    readonly property bool wifiPending: wifiRequest.running
    Timer {
        id: wifiRequest
        property bool desired: false
        interval: 4000
        onTriggered: {
            if (status.wifiEnabled !== desired)
                status.networkError = qsTr("Wi-Fi could not be changed in this login session.");
        }
    }
    onWifiEnabledChanged: {
        if (wifiEnabled === wifiRequest.desired) wifiRequest.stop();
        networkError = "";
    }
    function setWifiEnabled(enabled) {
        if (!wifiCanChange || wifiPending) return;
        networkError = "";
        wifiRequest.desired = enabled;
        wifiRequest.restart();
        networkHandler.enableWireless(enabled);
    }

    readonly property var sink: PA.PreferredDevice.sink
    readonly property bool audioAvailable: !!sink && sink.name !== "auto_null"
    readonly property bool muted: audioAvailable ? sink.muted : false
    readonly property int volumePercent: audioAvailable ? Math.round(sink.volume / PA.PulseAudio.NormalVolume * 100) : 0
    readonly property string audioDescription: audioAvailable ? sink.description : qsTr("No audio output available")
    property string audioError: ""
    Timer {
        id: audioRequest
        property int desiredVolume: -1
        property bool desiredMute: false
        interval: 1500
        onTriggered: {
            if (!status.audioAvailable || status.muted !== desiredMute ||
                (desiredVolume >= 0 && Math.abs(status.volumePercent - desiredVolume) > 1))
                status.audioError = qsTr("Audio could not be changed in this login session.");
        }
    }
    function setVolume(percent) {
        if (!audioAvailable) return;
        const value = Math.round(Math.max(0, Math.min(100, percent)));
        audioError = "";
        audioRequest.desiredVolume = value;
        audioRequest.desiredMute = value === 0;
        audioRequest.restart();
        sink.volume = Math.round(value / 100 * PA.PulseAudio.NormalVolume);
        sink.muted = value === 0;
    }
    function toggleMute() {
        if (!audioAvailable) return;
        audioError = "";
        audioRequest.desiredVolume = -1;
        audioRequest.desiredMute = !muted;
        audioRequest.restart();
        sink.muted = audioRequest.desiredMute;
    }

    readonly property bool hasBattery: battery.hasInternalBatteries
    readonly property int batteryPercent: battery.percent
    readonly property bool pluggedIn: battery.pluggedIn
    readonly property string batteryIcon: "battery-" + Math.round(batteryPercent / 10) * 10 + (pluggedIn ? "-charging" : "") + "-symbolic"
    readonly property string batterySummary: battery.state === Battery.BatteryControlModel.FullyCharged ? qsTr("Fully charged") :
        battery.state === Battery.BatteryControlModel.Charging ? qsTr("Charging") : pluggedIn ? qsTr("Plugged in") : qsTr("On battery power")
    readonly property string batteryRemaining: {
        const minutes = Math.round(battery.smoothedRemainingMsec / 60000);
        if (minutes <= 0 || battery.state === Battery.BatteryControlModel.FullyCharged) return "";
        const duration = minutes >= 60 ? qsTr("%1 h %2 min").arg(Math.floor(minutes / 60)).arg(minutes % 60) : qsTr("%1 min").arg(minutes);
        return battery.state === Battery.BatteryControlModel.Charging ? qsTr("%1 until full").arg(duration) : qsTr("About %1 remaining").arg(duration);
    }
}
