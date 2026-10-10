// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.networkmanagement as NM
import org.kde.bluezqt as BluezQt
import org.kde.plasma.private.volume as PA
import org.kde.plasma.private.brightnesscontrolplugin as Brightness
import org.kde.plasma.private.mpris as Mpris
import org.kde.notificationmanager as Notifications
import org.kde.plasma.private.sessions as Sessions
import org.kde.kcmutils

Item {
    id: backend
    visible: false
    NM.AvailableDevices { id: devices }
    NM.EnabledConnections { id: connections }
    NM.ConnectionIcon { id: connectionIcon }
    NM.NetworkStatus { id: network }
    NM.Handler { id: networkHandler }
    Notifications.Settings { id: notifications }
    Brightness.ScreenBrightnessControl { id: brightness; isSilent: true }
    Mpris.Mpris2Model { id: players }
    Sessions.SessionManagement { id: session }

    readonly property bool wifiAvailable: devices.wirelessDeviceAvailable
    readonly property bool wifiEnabled: connections.wirelessEnabled && connections.wirelessHwEnabled
    readonly property bool wifiToggleAllowed: wifiAvailable && connections.wirelessHwEnabled && !NM.Configuration.airplaneModeEnabled
    readonly property string networkIcon: wifiAvailable ? "network-wireless-symbolic" : connectionIcon.connectionIcon
    readonly property string networkName: wifiAvailable
        ? (wifiEnabled ? qsTr("On · Available networks") : qsTr("Off"))
        : (network.activeConnections.replace(/\n/g, " · ") || qsTr("Not connected"))
    readonly property bool bluetoothAvailable: BluezQt.Manager.adapters.length > 0
    readonly property bool bluetoothEnabled: BluezQt.Manager.bluetoothOperational
    readonly property string bluetoothName: BluezQt.Manager.connectedDevices.length
        ? BluezQt.Manager.connectedDevices.map(device => device.name).join(", ")
        : bluetoothEnabled ? qsTr("Ready to connect") : qsTr("Off")
    readonly property bool dndAvailable: Notifications.Server.valid
    readonly property bool dndEnabled: Notifications.Server.inhibited
    readonly property var sink: PA.PreferredDevice.sink
    readonly property var microphone: PA.PreferredDevice.source
    readonly property bool volumeAvailable: !!sink && sink.name !== "auto_null"
    readonly property bool muted: sink?.muted ?? false
    readonly property int volume: sink ? Math.round(sink.volume / PA.PulseAudio.NormalVolume * 100) : 0
    readonly property string outputName: sink?.description ?? qsTr("No audio output")
    readonly property bool microphoneAvailable: !!microphone
    readonly property bool microphoneMuted: microphone?.muted ?? false
    readonly property var displays: brightness.displays
    readonly property bool brightnessAvailable: brightness.isBrightnessAvailable
    readonly property var player: players.currentPlayer
    readonly property string track: player?.track || qsTr("Nothing playing")
    readonly property string artist: player?.artist || player?.identity || qsTr("Your media will appear here")
    readonly property string artwork: player?.artUrl ?? ""
    readonly property bool playing: player?.playbackStatus === Mpris.PlaybackStatus.Playing
    readonly property bool canPlayPause: !!player && (playing ? player.canPause : player.canPlay)
    readonly property bool canPrevious: player?.canGoPrevious ?? false
    readonly property bool canNext: player?.canGoNext ?? false
    readonly property bool canLock: session.canLock

    function toggleWifi() { if (wifiToggleAllowed) networkHandler.enableWireless(!wifiEnabled); }
    function toggleBluetooth() {
        const enable = !bluetoothEnabled;
        BluezQt.Manager.bluetoothBlocked = !enable;
        for (const adapter of BluezQt.Manager.adapters) adapter.powered = enable;
    }
    function toggleDnd() {
        if (dndEnabled) {
            notifications.notificationsInhibitedUntil = undefined;
            notifications.revokeApplicationInhibitions();
            notifications.screensMirrored = false;
        } else {
            const until = new Date();
            until.setFullYear(until.getFullYear() + 1);
            notifications.notificationsInhibitedUntil = until;
        }
        notifications.save();
    }
    function toggleMicrophone() { if (microphone) microphone.muted = !microphone.muted; }
    function toggleMute() { if (sink) sink.muted = !sink.muted; }
    function setVolume(value) {
        if (!sink) return;
        sink.volume = Math.round(value * PA.PulseAudio.NormalVolume / 100);
        if (value > 0 && sink.muted) sink.muted = false;
    }
    function setBrightness(display, value) { brightness.setBrightness(display, Math.round(value)); }
    function playPause() { if (canPlayPause) playing ? player.Pause() : player.Play(); }
    function previous() { if (canPrevious) player.Previous(); }
    function next() { if (canNext) player.Next(); }
    function lock() { if (canLock) session.lock(); }
    function powerOptions() { session.requestLogoutPrompt(); }
    function settings() { KCMLauncher.openSystemSettings("kcm_landingpage"); }
}
