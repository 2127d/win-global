// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../details" as Details

Rectangle {
    id: preview
    width: 442; height: Math.min(790,view.naturalHeight + 76)
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property alias drive: drive
    property alias view: view
    QtObject {
        id: fixture
        property int devicesModel: 1
        property string filter: "removable"
        property int unmountableCount: 1
        property bool settingsAllowed: true
        property bool autoPopup: true
        property int settingsCalls: 0
        property int removeCalls: 0
        function setFilter(value) { filter=value; }
        function settings() { settingsCalls++; }
        function removeAll() { removeCalls++; }
        function setAutoPopup(value) { autoPopup=value; }
    }
    QtObject {
        id: drive
        property string name: "Travel drive"
        property string kind: "USB storage"
        property string icon: "drive-removable-media-usb"
        property string emblem: "emblem-mounted"
        property bool busy: false
        property bool canAct: !busy
        property bool failed: false
        property string status: "Ready to use"
        property bool capacityVisible: true
        property real usedFraction: 0.68
        property bool lowSpace: usedFraction >= 0.95
        property string capacityText: "82 GB free of 256 GB"
        property string defaultText: "Safely remove"
        property string defaultIcon: "media-eject-symbolic"
        property var actionsModel: [
            {Name:"open",Text:"Open in File Manager",Icon:"system-file-manager"},
            {Name:"mount",Text:"Mount",Icon:"media-mount"},
            {Name:"check",Text:"Check for errors",Icon:"tools-check-spelling"},
            {Name:"repair",Text:"Repair filesystem",Icon:"tools-wizard"}
        ]
        property int defaultCalls: 0
        property string lastAction: ""
        function triggerDefault() { defaultCalls++; }
        function triggerAction(name) { lastAction=name; }
    }
    Details.DetailSurface {
        objectName: "diskHeading"
        x: 12; y: 12; width: parent.width - 24; height: 38; radius: 19; elevated: true
        Kirigami.Heading { anchors.centerIn: parent; text: "Disks & Devices"; level: 3 }
    }
    DevicesView {
        id: view; x: 12; y: 60; width: parent.width - 24; height: parent.height - y - 12
        controller: fixture
        deviceDelegate: Component { DeviceCard { Layout.fillWidth: true; controller: drive } }
    }
}
