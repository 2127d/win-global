// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../details" as Details

Rectangle {
    id: preview
    width: 442; height: Math.min(790, view.naturalHeight + 76)
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property alias view: view
    property bool expanded: false
    QtObject {
        id: fixture
        property int deviceCount: 1
        property int pairedCount: 1
        property int discoveredCount: 1
        property string name: "Pixel 9"
        property string icon: "smartphone-symbolic"
        property bool reachable: true
        property bool batteryVisible: true
        property string batteryText: "78%"
        property string batteryIcon: "battery-080"
        property bool signalVisible: true
        property string signalText: "5G · 4/4"
        property string signalIcon: "network-cellular-signal-excellent"
        property bool shareAvailable: true
        property bool clipboardAvailable: true
        property bool virtualActive: false
        property var actions: [
            {id:"share", label:"Send files", icon:"document-share", available:shareAvailable},
            {id:"browse", label:"Browse files", icon:"folder-open-symbolic", available:true},
            {id:"clipboard", label:"Send clipboard", icon:"edit-paste-symbolic", available:clipboardAvailable},
            {id:"sms", label:"Messages", icon:"message-new", available:true},
            {id:"ring", label:"Ring device", icon:"notifications-symbolic", available:true},
            {id:"display", label:"Virtual display", icon:"video-display-symbolic", available:true,checked:virtualActive}
        ]
        property var notificationModel: [
            {appIcon:"mail-unread",appName:"Messages",title:"Alex",notitext:"See you at the café at 3?",repliable:true,dismissable:true},
            {appIcon:"calendar",appName:"Calendar",title:"Design review",notitext:"Today, 4:00–4:30 PM",repliable:false,dismissable:false}
        ]
        readonly property int notificationCount: notificationModel.length
        property bool dismissable: true
        property var commandModel: [{key:"presentation",name:"Start presentation",command:"Open the presentation on your other device"},{key:"lock",name:"Lock computer",command:"loginctl lock-session"}]
        readonly property int commandCount: commandModel.length
        property bool commandsAvailable: true
        property bool canAddCommand: true
        property Component keyboard: null
        property string lastAction: ""
        property var sentFiles: []
        property int dismissCalls: 0
        property int dismissAllCalls: 0
        property int replyCalls: 0
        property string lastReply: ""
        property string lastCommand: ""
        property int addCalls: 0
        property int settingsCalls: 0
        property string lastStore: ""
        function activate(id) { lastAction=id; }
        function sendFiles(urls) { sentFiles=urls; }
        function dismiss(note) { dismissCalls++; }
        function dismissAll() { dismissAllCalls++; }
        function reply(note,text) { lastReply=text; replyCalls++; }
        function runCommand(key) { lastCommand=key; }
        function addCommand() { addCalls++; }
    }
    Details.DetailSurface {
        id: heading; objectName: "connectHeading"
        x: 12; y: 12; width: parent.width - 24; height: 38; radius: 19; elevated: true
        Kirigami.Heading { anchors.centerIn: parent; text: "KDE Connect"; level: 3 }
    }
    ConnectView {
        id: view
        x: 12; y: 60; width: parent.width - 24; height: parent.height - y - 12
        devicesModel: fixture.deviceCount
        pairedCount: fixture.pairedCount; discoveredCount: fixture.discoveredCount
        deviceDelegate: Component {
            ConnectDeviceView {
                Layout.fillWidth: true
                controller: fixture
                notificationsExpanded: preview.expanded
                commandsExpanded: preview.expanded
            }
        }
        onSettingsRequested: fixture.settingsCalls++
        onStoreRequested: url => fixture.lastStore=url
    }
}
