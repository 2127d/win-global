// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.bluezqt as BluezQt
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import org.kde.plasma.private.bluetooth as PlasmaBt
import org.kde.bluedevil.components as BluedevilComponents
import org.kde.kquickcontrolsaddons as KQuickControlsAddons
import "BluetoothDevices.js" as Devices

PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaDetailPage"
    required property PlasmoidItem plasmoidItem
    required property PlasmaCore.Action addDeviceAction
    required property PlasmaCore.Action toggleBluetoothAction
    required property PlasmaCore.Action toggleBadgeAction
    required property PlasmaCore.Action configureAction
    property var records: []
    readonly property int navigationDepth: spotlight.navigationDepth
    function goBack() { spotlight.goBack(); }
    padding: 0
    background: null
    focus: true
    collapseMarginsHint: true
    implicitWidth: 418
    implicitHeight: 470
    readonly property real sequoiaPreferredHeight: spotlight.naturalHeight
    Keys.forwardTo: [spotlight]

    function refresh() {
        const next = [];
        for (let i = 0; i < deviceRecords.count; ++i) {
            const entry = deviceRecords.objectAt(i)?.snapshot;
            if (entry && (entry.paired || entry.connected)) next.push(entry);
        }
        records = next;
    }
    function record(id) { return records.find(d => d.id === id) || null; }

    PlasmaBt.DevicesProxyModel {
        id: devicesModel
        hideBlockedDevices: true
        sourceModel: PlasmaBt.SharedDevicesStateProxyModel
    }
    Instantiator {
        id: deviceRecords
        model: devicesModel
        delegate: QtObject {
            required property var model
            readonly property var snapshot: ({
                id: model.Ubi,
                name: model.DeviceFullName,
                icon: model.Icon,
                device: model.Device,
                address: model.Address,
                paired: model.Paired,
                trusted: model.Trusted,
                adapterName: model.Adapter?.name ?? "",
                connected: model.Connected,
                connecting: model.Connecting,
                disconnecting: model.Disconnecting,
                busy: model.Connecting || model.Disconnecting,
                failed: model.ConnectionFailed,
                audio: [BluezQt.Device.Headset, BluezQt.Device.Headphones, BluezQt.Device.OtherAudio].includes(model.Type)
                    || model.Uuids.indexOf(BluezQt.Services.AdvancedAudioDistribution) !== -1,
                battery: model.Battery ? Devices.battery(model.Battery.percentage) : -1,
                mediaPlayer: model.MediaPlayer,
                playing: !!root.plasmoidItem.sequoiaActivity.playing[model.Ubi],
                transferring: !!root.plasmoidItem.sequoiaActivity.transferring[model.Address.toUpperCase()],
                canBrowse: model.Uuids.indexOf(BluezQt.Services.ObexFileTransfer) !== -1,
                canSend: model.Uuids.indexOf(BluezQt.Services.ObexObjectPush) !== -1
            })
            onSnapshotChanged: Qt.callLater(root.refresh)
        }
        onObjectAdded: Qt.callLater(root.refresh)
        onObjectRemoved: Qt.callLater(root.refresh)
    }
    BluedevilComponents.ForgetDeviceDialog {
        id: forgetDialog
        parent: root
        registerCallForDeviceUbi: (call, ubi) => PlasmaBt.SharedDevicesStateProxyModel.registerDisconnectingCallForDeviceUbi(call, ubi)
    }
    KQuickControlsAddons.Clipboard { id: clipboard }
    QtObject {
        id: nativeController
        readonly property var devices: root.records
        readonly property var connectionOrder: root.plasmoidItem.sequoiaActivity.connectionOrder
        readonly property bool operational: BluezQt.Manager.bluetoothOperational
        readonly property bool adapterAvailable: BluezQt.Manager.rfkill.state !== BluezQt.Rfkill.Unknown
        function setEnabled(enabled) { root.plasmoidItem.setBluetoothEnabled(enabled); }
        function pair() { if (operational) root.addDeviceAction.trigger(); }
        function settings() { root.configureAction.trigger(); }
        function toggleDevice(id) {
            const item = root.record(id);
            if (!operational || !item?.device || item.busy) return;
            if (item.connected) {
                const call = item.device.disconnectFromDevice();
                PlasmaBt.SharedDevicesStateProxyModel.registerDisconnectingCallForDeviceUbi(call, id);
            } else {
                const call = item.device.connectToDevice();
                PlasmaBt.SharedDevicesStateProxyModel.registerConnectingCallForDeviceUbi(call, id);
            }
        }
        function forget(id) { const item = root.record(id); if (item?.device && !item.busy) forgetDialog.open(item.device); }
        function copyAddress(id) { const item = root.record(id); if (item) clipboard.content = item.address; }
        function browse(id) { const item = root.record(id); if (item?.canBrowse) Qt.openUrlExternally("obexftp://" + item.address.replace(/:/g, "-") + "/"); }
        function send(id) { const item = root.record(id); if (item?.canSend) PlasmaBt.LaunchApp.launchSendFile(id); }
    }
    BluetoothSpotlightView {
        id: spotlight
        anchors.fill: parent
        controller: nativeController
        showPowerControl: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading)
        mediaComponent: Component {
            MediaPlayerItem { mediaPlayer: parent.deviceRecord?.mediaPlayer ?? null }
        }
    }
}
