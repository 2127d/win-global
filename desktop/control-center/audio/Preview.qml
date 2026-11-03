// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Rectangle {
    id: preview
    width: 418; height: 560
    color: Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    property alias view: spotlight
    property alias fixture: fixture
    QtObject {
        id: fixture
        property var outputs: []
        property var inputs: []
        property var playback: []
        property var recording: []
        property string layoutMode: "spotlight"
        property string outputId: "headphones"
        property string inputId: "microphone"
        property bool ready: true
        property bool settingsAllowed: true
        property int maximumVolume: 100
        property int volumeStep: 5
        property string lastAction: ""
        function reset() {
            layoutMode = "spotlight";
            outputs = [
                {id:"headphones",index:1,type:"sink",name:"Heavys H1H",subtitle:"Bluetooth headphones",icon:"audio-headphones-symbolic",volume:61,muted:false,hasVolume:true,writable:true},
                {id:"speakers",index:2,type:"sink",name:"Built-in speakers",subtitle:"Internal audio",icon:"audio-speakers-symbolic",volume:40,muted:false,hasVolume:true,writable:true}
            ];
            inputs = [{id:"microphone",index:3,type:"source",name:"Built-in microphone",subtitle:"",volume:72,muted:false,hasVolume:true,writable:true}];
            playback = [
                {id:"browser",deviceIndex:1,type:"sink-input",name:"Firefox",icon:"firefox",volume:80,muted:false,hasVolume:true,writable:true},
                {id:"chat",deviceIndex:1,type:"sink-input",name:"Discord",icon:"discord",volume:65,muted:false,hasVolume:true,writable:true}
            ];
            recording = []; outputId="headphones";inputId="microphone";ready=true;maximumVolume=100;lastAction="";
        }
        function change(id, values) {
            for (const key of ["outputs","inputs","playback","recording"]) fixture[key] = fixture[key].map(d => d.id === id ? Object.assign({},d,values) : d);
        }
        function find(id) { return outputs.concat(inputs,playback,recording).find(d=>d.id===id); }
        function setVolume(id, percent) { lastAction="volume:"+id; change(id,{volume:percent,muted:percent===0}); }
        function toggleMute(id) { lastAction="mute:"+id; change(id,{muted:!find(id).muted}); }
        function selectDevice(id) { lastAction="default:"+id; if(find(id).type==="sink")outputId=id;else inputId=id; }
        function setLayout(mode) { if (["spotlight","channels","mixer"].includes(mode)) {layoutMode=mode;lastAction="layout:"+mode;} }
        function routeStream(id, destinationId) { const target=find(destinationId);if(target){change(id,{deviceIndex:target.index});lastAction="route:"+id+":"+destinationId;} }
        function feedback(id) {}
        function retry() { lastAction="retry"; }
        function settings() { lastAction="settings"; }
        function more(button) { lastAction="more"; }
        function options(id,button) { lastAction="options:"+id; optionsMenu.openFor(id,button); }
        function optionEntries(id) {
            if (!find(id)) return [];
            return [{label:"Profiles",section:true},{key:"profile:stereo",label:"High fidelity playback",action:"profile",name:"stereo",checkable:true,checked:true},{key:"profile:headset",label:"Headset & microphone",action:"profile",name:"headset",checkable:true,checked:false}];
        }
        function activateOption(id,entry) { lastAction="option:"+id+":"+entry.name; }
        Component.onCompleted: reset()
    }
    AudioOptionsMenu { id: optionsMenu; controller: fixture }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12
        Details.DetailSurface {
            Layout.fillWidth: true; implicitHeight: heading.implicitHeight + 8; radius: height / 2; elevated: true
            RowLayout {
                id: heading; anchors.fill: parent; anchors.margins: 4
                AudioActionButton { flat: true; icon.name: "go-previous-symbolic"; text: qsTr("Back"); display: PC.AbstractButton.IconOnly }
                PC.Label { text: qsTr("Audio"); Layout.fillWidth: true; font.weight: Font.DemiBold }
                AudioActionButton { flat: true; icon.name: "window-pin-symbolic"; text: qsTr("Keep open"); display: PC.AbstractButton.IconOnly; checkable: true }
            }
        }
        AudioSpotlightView { id: spotlight; Layout.fillWidth: true; Layout.fillHeight: true; controller: fixture }
    }
}
