// SPDX-License-Identifier: MIT
import QtQuick
import plasma.applet.org.kde.plasma.devicenotifier as NativeDevices

Item {
    id: device
    required property int index
    required property var appletInterface
    required property string deviceDescription
    required property string deviceType
    required property string deviceIcon
    required property double deviceFreeSpace
    required property double deviceSize
    required property string deviceFreeSpaceText
    required property string deviceSizeText
    required property bool deviceIsBusy
    required property bool deviceMounted
    required property int deviceState
    required property int deviceOperationResult
    required property string deviceMessage
    required property var deviceActions
    required property var deviceEmblems
    signal messageShown()
    implicitHeight: card.implicitHeight
    readonly property bool free: !deviceIsBusy && deviceState !== NativeDevices.StateInfo.NotPresent
    readonly property bool finished: free && deviceState !== NativeDevices.StateInfo.Idle
    onFinishedChanged: if (finished) appletInterface.showOperationFeedback(deviceOperationResult === 0)
    onDeviceStateChanged: if (deviceState === NativeDevices.StateInfo.Unmounting) unmountTimer.restart()
    onDeviceMessageChanged: if (deviceMessage.length > 0) { attentionTimer.restart(); messageShown(); }
    Timer { id: unmountTimer; interval: 1000 }
    Timer { id: attentionTimer; interval: 6000 }
    QtObject {
        id: deviceController
        readonly property string name: device.deviceDescription
        readonly property string kind: device.deviceType
        readonly property string icon: device.deviceIcon
        readonly property string emblem: device.finished && device.deviceMessage.length > 0 ? (device.deviceOperationResult === 0 ? "emblem-information" : "emblem-error") : !busy ? (device.deviceEmblems[0] ?? "") : ""
        readonly property bool busy: device.deviceIsBusy
        readonly property bool canAct: device.free
        readonly property bool failed: device.finished && device.deviceMessage.length > 0 && device.deviceOperationResult !== 0
        readonly property string status: {
            if (device.finished && device.deviceMessage.length > 0) return device.deviceMessage;
            if (device.deviceState === NativeDevices.StateInfo.Checking) return qsTr("Checking for errors…");
            if (device.deviceState === NativeDevices.StateInfo.Repairing) return qsTr("Repairing the filesystem…");
            if (device.deviceState === NativeDevices.StateInfo.Unmounting) return unmountTimer.running ? qsTr("Safely removing…") : qsTr("Don’t unplug yet. Files are still being transferred…");
            if (device.deviceState === NativeDevices.StateInfo.Mounting) return qsTr("Accessing device…");
            if (busy) return qsTr("Working…");
            if (device.deviceState === NativeDevices.StateInfo.NotPresent) return device.deviceMessage || qsTr("Device is no longer available");
            return device.deviceMounted ? qsTr("Ready to use") : qsTr("Not mounted");
        }
        readonly property bool capacityVisible: device.deviceMounted && device.deviceSize > 0 && device.deviceFreeSpace >= 0
        readonly property real usedFraction: device.deviceSize > 0 ? 1-device.deviceFreeSpace/device.deviceSize : 0
        readonly property bool lowSpace: capacityVisible && device.deviceFreeSpace/device.deviceSize <= 0.05
        readonly property string capacityText: qsTr("%1 free of %2").arg(device.deviceFreeSpaceText).arg(device.deviceSizeText)
        readonly property var actionsModel: device.deviceActions
        readonly property string defaultText: device.deviceActions?.defaultActionText ?? ""
        readonly property string defaultIcon: device.deviceActions?.defaultActionIcon ?? ""
        function triggerDefault() {
            if (!canAct || !device.deviceActions) return;
            if (device.deviceMounted) unmountTimer.restart();
            device.deviceActions.actionTriggered(device.deviceActions.defaultActionName);
        }
        function triggerAction(name) { if (canAct && device.deviceActions) device.deviceActions.actionTriggered(name); }
    }
    DeviceCard { id: card; width: parent.width; controller: deviceController; attention: attentionTimer.running }
}
