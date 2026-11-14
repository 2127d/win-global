// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
    property alias groupedView:groupedView
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
                clip:true;contentWidth:width;contentHeight:body.height;boundsBehavior:Flickable.StopAtBounds
                Column {
                    id:body;width:parent.width;spacing:8
                    NetworkSummary {id:summary;width:parent.width;layoutMode:fixture.layoutMode;entries:fixture.entries;onConnectionRequested:row=>fixture.lastAction="details:"+row;onConfigureRequested:fixture.settings()}
                    Details.DetailSurface {
                        visible:fixture.layoutMode === "desk" || fixture.layoutMode === "compact"
                        width:parent.width;height:groupedView.contentHeight;radius:24
                        ListView {
                            id:groupedView;anchors.fill:parent;interactive:false;spacing:3
                            footer: Item { width: groupedView.width; height: 12 }
                            header:Column {
                                width:groupedView.width
                                Item {width:parent.width;height:64;Details.DetailSearchField {x:12;y:8;width:parent.width-24;height:48;placeholderText:"Search networks…"}}
                                PC.Label {text:"Available";leftPadding:16;topPadding:14;bottomPadding:6;color:Kirigami.Theme.disabledTextColor}
                            }
                            model:[{name:"Pixel 9a Network",type:PlasmaNM.Enums.Bluetooth,icon:"network-bluetooth"},
                                   {name:"xfinitywifi",type:PlasmaNM.Enums.Wireless,icon:"network-wireless-symbolic"},
                                   {name:"Xfinity Mobile",type:PlasmaNM.Enums.Wireless,icon:"network-wireless-encrypted-symbolic"}]
                            delegate:NetworkListItem {
                                required index
                                required property var modelData
                                objectName:"networkGrouped:"+index
                                width:groupedView.width-24;grouped:true
                                connectionType:modelData.type;connectionIcon:modelData.icon
                                title:modelData.name;subtitle:index===0?"Never used":""
                                defaultActionButtonAction:Action {text:"Connect";icon.name:"network-connect-symbolic";onTriggered:fixture.lastAction="connect:"+index}
                                contextualActions: [
                                    Action {text:"Configure…";icon.name:"configure";onTriggered:fixture.lastAction="configure:"+index},
                                    Action {text:"Share";icon.name:"view-barcode-qr";onTriggered:fixture.lastAction="share:"+index}
                                ]
                                customExpandedViewContent:Component {PC.Label {text:"Connection details"}}
                            }
                        }
                    }
                }
            }
        }
        NetworkActions {id:island;Layout.fillWidth:true;controller:fixture;layoutMode:fixture.layoutMode;onLayoutSelected:mode=>fixture.layoutMode=mode}
    }
}
