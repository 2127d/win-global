// SPDX-License-Identifier: MIT
import QtCore
import QtQuick
import QtQuick.Dialogs as QtDialogs
import org.kde.kdeconnect as KDEConnect
import org.kde.kirigami as Kirigami

Item {
    id: root
    required property int index
    required property var model
    implicitHeight: deviceView.implicitHeight
    readonly property KDEConnect.DeviceDbusInterface device: KDEConnect.DeviceDbusInterfaceFactory.create(model.deviceId)
    Battery { id: battery; device: root.device }
    Clipboard { id: clipboard; device: root.device }
    Connectivity { id: connectivity; device: root.device }
    FindMyPhone { id: findPhone; device: root.device }
    RemoteCommands { id: remoteCommands; device: root.device }
    Sftp { id: sftp; device: root.device }
    Share { id: share; device: root.device }
    SMS { id: sms; device: root.device }
    VirtualMonitor { id: virtualMonitor; device: root.device }
    KDEConnect.NotificationsModel { id: notifications; deviceId: root.model.deviceId }
    KDEConnect.RemoteCommandsModel { id: commands; deviceId: root.model.deviceId }
    QtDialogs.FileDialog {
        id: fileDialog
        title: qsTr("Choose files to send")
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        modality: Qt.NonModal; fileMode: QtDialogs.FileDialog.OpenFiles
        onAccepted: if (share.available) share.plugin.shareUrls(selectedFiles)
    }
    Kirigami.PromptDialog {
        id: virtualError
        title: qsTr("Virtual display is not available")
        showCloseButton: true; standardButtons: Kirigami.Dialog.NoButton
    }
    QtObject {
        id: nativeController
        readonly property string name: root.model.name
        readonly property string icon: root.device.type === "phone" ? "smartphone-symbolic" : root.device.type === "tablet" ? "tablet-symbolic" : "computer-symbolic"
        readonly property bool reachable: root.device.isReachable
        readonly property bool batteryVisible: battery.available && battery.hasBattery
        readonly property string batteryText: battery.displayString
        readonly property string batteryIcon: battery.iconName
        readonly property bool signalVisible: connectivity.available
        readonly property string signalText: connectivity.displayString
        readonly property string signalIcon: connectivity.iconName
        readonly property bool shareAvailable: share.available
        readonly property var actions: [
            {id:"share", label:qsTr("Send files"), icon:"document-share", available:share.available},
            {id:"browse", label:qsTr("Browse files"), icon:"folder-open-symbolic", available:sftp.available},
            {id:"clipboard", label:qsTr("Send clipboard"), icon:"edit-paste-symbolic", available:clipboard.clipboard?.isAutoShareDisabled ?? false},
            {id:"sms", label:qsTr("Messages"), icon:"message-new", available:sms.available},
            {id:"ring", label:qsTr("Ring device"), icon:"notifications-symbolic", available:findPhone.available},
            {id:"display", label:qsTr("Virtual display"), icon:"video-display-symbolic", available:virtualMonitor.available, checked:virtualMonitor.plugin?.active ?? false}
        ]
        readonly property var notificationModel: notifications
        readonly property int notificationCount: notifications.count
        readonly property bool dismissable: notifications.isAnyDimissable
        readonly property var commandModel: commands
        readonly property int commandCount: remoteCommands.available ? commands.count : 0
        readonly property bool commandsAvailable: remoteCommands.available
        readonly property bool canAddCommand: remoteCommands.plugin?.canAddCommand ?? false
        readonly property Component keyboard: Component { NativeKeyboard { device: root.device } }
        function activate(id) {
            if (!reachable || !actions.some(a => a.id === id && a.available)) return;
            switch (id) {
            case "share": fileDialog.open(); break;
            case "browse": sftp.browse(); break;
            case "clipboard": clipboard.sendClipboard(); break;
            case "sms": sms.plugin.launchApp(); break;
            case "ring": findPhone.ring(); break;
            case "display":
                if (virtualMonitor.plugin.active) { virtualMonitor.plugin.stop(); virtualError.close(); }
                else {
                    virtualMonitor.plugin.requestVirtualMonitor();
                    virtualError.subtitle = virtualMonitor.plugin.lastError;
                    if (virtualError.subtitle.length > 0) virtualError.open();
                }
                break;
            }
        }
        function sendFiles(urls) {
            if (!reachable || !share.available) return;
            const unique = [...new Set(urls.map(url => url.toString()))];
            unique.forEach(url => share.plugin.shareUrl(url));
        }
        function dismiss(note) { if (reachable && note.dismissable) note.dbusInterface.dismiss(); }
        function dismissAll() { if (reachable && dismissable) notifications.dismissAll(); }
        function reply(note, text) { if (reachable && note.repliable && text.length > 0) note.dbusInterface.sendReply(text); }
        function runCommand(key) { if (reachable && remoteCommands.available) remoteCommands.plugin.triggerCommand(key); }
        function addCommand() { if (reachable && canAddCommand) remoteCommands.plugin.editCommands(); }
    }
    ConnectDeviceView { id: deviceView; width: parent.width; controller: nativeController }
}
