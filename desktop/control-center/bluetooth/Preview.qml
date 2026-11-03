// SPDX-License-Identifier: MIT
// Isolated fixture; never installed in a widget.
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Rectangle {
    id: preview
    width: 418; height: 520
    color: Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    property alias view: spotlight
    property alias fixture: fixture
    QtObject {
        id: fixture
        property bool operational: true
        property bool adapterAvailable: true
        property string lastAction: ""
        property var devices: []
        function reset() {
            operational = true; adapterAvailable = true; lastAction = "";
            devices = [
                {id:"headphones",name:"Heavys H1H",icon:"audio-headphones-symbolic",address:"AA:BB:CC:DD:EE:01",adapterName:"Desktop adapter",connected:true,paired:true,trusted:true,audio:true,battery:76,busy:false,connecting:false,disconnecting:false,failed:false,canBrowse:false,canSend:false,mediaPlayer:null},
                {id:"mouse",name:"MX Master 3S",icon:"input-mouse-symbolic",address:"AA:BB:CC:DD:EE:02",adapterName:"Desktop adapter",connected:true,paired:true,trusted:true,audio:false,battery:92,busy:false,connecting:false,disconnecting:false,failed:false,canBrowse:false,canSend:false,mediaPlayer:null},
                {id:"keyboard",name:"Keychron K3",icon:"input-keyboard-symbolic",address:"AA:BB:CC:DD:EE:03",adapterName:"Desktop adapter",connected:false,paired:true,trusted:false,audio:false,battery:-1,busy:false,connecting:false,disconnecting:false,failed:false,canBrowse:false,canSend:false,mediaPlayer:null}
            ];
        }
        function change(id, values) { devices = devices.map(d => d.id === id ? Object.assign({},d,values) : d); }
        function setEnabled(value) { operational = value; lastAction = "power:" + value; }
        function toggleDevice(id) {
            const d = devices.find(d => d.id === id);
            if (!operational || !d || d.busy) return;
            lastAction = (d.connected ? "disconnect:" : "connect:") + id;
            change(id,{busy:true,connecting:!d.connected,disconnecting:d.connected,failed:false});
        }
        function finish(id, failed) {
            const d = devices.find(d => d.id === id); if (!d) return;
            change(id,{connected:failed?d.connected:!d.connected,busy:false,connecting:false,disconnecting:false,failed:failed});
        }
        function pair() { lastAction = "pair"; }
        function settings() { lastAction = "settings"; }
        function forget(id) { lastAction = "forget:" + id; }
        function copyAddress(id) { lastAction = "copy:" + id; }
        function browse(id) { lastAction = "browse:" + id; }
        function send(id) { lastAction = "send:" + id; }
        Component.onCompleted: reset()
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12
        spacing: 12
        Details.DetailSurface {
            Layout.fillWidth: true
            implicitHeight: heading.implicitHeight + 8
            radius: height / 2
            elevated: true
            RowLayout {
                id: heading
                anchors.fill: parent; anchors.margins: 4
                Details.DetailToolButton { icon.name: "go-previous-symbolic"; text: qsTr("Back"); display: PC.AbstractButton.IconOnly; onClicked: spotlight.goBack() }
                PC.Label { text: qsTr("Bluetooth"); Layout.fillWidth: true; font.weight: Font.DemiBold }
                PC.Switch { objectName: "bluetoothHeaderSwitch"; Accessible.name: qsTr("Bluetooth"); checked: fixture.operational; enabled: fixture.adapterAvailable; onToggled: fixture.setEnabled(checked) }
            }
        }
        BluetoothSpotlightView {
            id: spotlight
            Layout.fillWidth: true; Layout.fillHeight: true
            controller: fixture
            showPowerControl: false
        }
    }
}
