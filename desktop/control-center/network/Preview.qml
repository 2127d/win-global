// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.kirigami as Kirigami
import "../details" as Details
Rectangle {
    id:preview;width:418;height:680;color:Kirigami.Theme.backgroundColor
    property alias fixture:fixture
    property alias summary:summary
    property alias island:island
    property alias scroll:scroll
    QtObject {
        id:fixture
        property string layoutMode:"spotlight"
        property bool hotspotAvailable:true
        property bool hotspotSupported:true
        property bool hotspotActive:false
        property bool qrAvailable:true
        property bool settingsAllowed:true
        property string hotspotHint:"Create Hotspot"
        property string lastAction:""
        property var entries:[]
        function reset() {
            layoutMode="spotlight";hotspotSupported=true;hotspotActive=false;qrAvailable=true;settingsAllowed=true;lastAction="";
            entries=[{row:0,name:"Home network",icon:"network-wireless-symbolic",type:PlasmaNM.Enums.Wireless,active:true,connecting:false},{row:1,name:"Ethernet",icon:"network-wired-symbolic",type:PlasmaNM.Enums.Wired,active:true,connecting:false},{row:2,name:"Work VPN",icon:"network-vpn-symbolic",type:PlasmaNM.Enums.Vpn,active:true,connecting:false}];
        }
        function toggleHotspot(){lastAction="hotspot";hotspotActive=!hotspotActive;}
        function scanQr(){lastAction="qr";}
        function settings(){lastAction="settings";}
        function scan(){lastAction="scan";}
        Component.onCompleted:reset()
    }
    ColumnLayout {
        anchors.fill:parent;anchors.margins:12;spacing:12
        Details.DetailSurface {Layout.fillWidth:true;implicitHeight:42;radius:21;elevated:true
            RowLayout {anchors.fill:parent;anchors.margins:4;NetworkActionButton{flat:true;icon.name:"go-previous-symbolic";display:PC.AbstractButton.IconOnly;text:"Back"} PC.Label{Layout.fillWidth:true;text:"Networks";font.weight:Font.DemiBold} NetworkActionButton{flat:true;display:PC.AbstractButton.IconOnly;icon.name:"window-pin-symbolic";text:"Pin"}}
        }
        PC.ScrollView {
            id:scroll
            Layout.fillWidth:true;Layout.fillHeight:true;background:null
            contentItem:Flickable {
                clip:true;contentWidth:width;contentHeight:summary.height;boundsBehavior:Flickable.StopAtBounds
                NetworkSummary {id:summary;width:parent.width;layoutMode:fixture.layoutMode;entries:fixture.entries;onConnectionRequested:row=>fixture.lastAction="details:"+row;onConfigureRequested:fixture.settings()}
            }
        }
        NetworkActions {id:island;Layout.fillWidth:true;controller:fixture;layoutMode:fixture.layoutMode;onLayoutSelected:mode=>fixture.layoutMode=mode}
    }
}
