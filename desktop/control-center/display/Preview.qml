// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Rectangle {
    id:preview;width:418;height:680;color:Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled:Qt.application.layoutDirection===Qt.RightToLeft
    LayoutMirroring.childrenInherit:true
    property alias view:view
    property alias fixture:fixture
    QtObject {
        id:fixture
        property string layoutMode:"spotlight"
        function setLayout(mode) { layoutMode=mode; }
        property var displays:[]
        property bool keyboardAvailable:false
        property bool keyboardColorAvailable:false
        property var keyboardRecord:({id:"keyboard",name:"Keyboard Backlight",brightness:2,maximum:3})
        property bool darkMode:false
        readonly property string currentTheme:darkMode?"Sequoia Eleven Dark":"Sequoia Eleven Light"
        readonly property string otherTheme:darkMode?"Sequoia Eleven Light":"Sequoia Eleven Dark"
        property bool settingsAllowed:true
        property bool themeSettingsAllowed:true
        property string statusMessage:""
        property string lastAction:""
        function reset(count) {
            const names=["Main display","Left display","Right display","Portrait display","Overhead display","Projector","Drawing tablet"];
            displays=Array.from({length:count??5},(_,i)=>({id:"screen"+i,backendId:"display"+i,number:i+1,name:names[i] || "Display "+(i+1),brightness:72-i*6,maximum:100}));
            layoutMode="spotlight";darkMode=false;keyboardAvailable=false;keyboardColorAvailable=false;settingsAllowed=true;themeSettingsAllowed=true;statusMessage="";lastAction="";
        }
        function setBrightness(id,value) {
            if(!Number.isFinite(value))return;
            if(id==="keyboard"){keyboardRecord=Object.assign({},keyboardRecord,{brightness:Math.round(Math.max(0,Math.min(keyboardRecord.maximum,value)))});lastAction="brightness:keyboard";return;}
            const d=displays.find(d=>d.id===id);if(!d || d.maximum<=0)return;
            lastAction="brightness:"+id;
            displays=displays.map(d=>d.id===id?Object.assign({},d,{brightness:Math.round(Math.max(0,Math.min(d.maximum,value)))}):d);
        }
        function setDarkMode(value){darkMode=value;lastAction="theme:"+value;}
        function settings(){lastAction="settings";}
        function themeSettings(){lastAction="themeSettings";}
        function identify(){lastAction="identify";}
        Component.onCompleted:reset(5)
    }
    ColumnLayout {
        anchors.fill:parent;anchors.margins:12;spacing:12
        Details.DetailSurface {
            Layout.fillWidth:true;implicitHeight:42;radius:21;elevated:true
            RowLayout {anchors.fill:parent;anchors.margins:4;DisplayActionButton{flat:true;display:PC.AbstractButton.IconOnly;icon.name:"go-previous-symbolic";text:"Back"} PC.Label{text:"Display";Layout.fillWidth:true;font.weight:Font.DemiBold} DisplayActionButton{flat:true;display:PC.AbstractButton.IconOnly;icon.name:"window-pin-symbolic";text:"Pin"}}
        }
        DisplaySpotlightView {
            id:view;Layout.fillWidth:true;Layout.fillHeight:true;controller:fixture
            nightLightCard: Component {
                DisplayNightLight {
                    control:nightFixture
                    onConfigureRequested:fixture.lastAction="nightSettings"
                    onToggleRequested:nightFixture.inhibited=!nightFixture.inhibited
                }
            }
        }
    }
    QtObject {
        id:nightFixture
        property bool available:true
        property bool enabled:true
        property bool running:true
        property bool inhibited:false
        property bool togglable:true
        property bool hasSwitchingTimes:true
        property bool daylight:false
        property bool transitioning:false
        property int currentTemperature:4200
        property double scheduledTransitionStartTime:new Date(2026,10,3,6,30).getTime()
        property double currentTransitionEndTime:scheduledTransitionStartTime
    }
    property alias nightFixture:nightFixture
}
