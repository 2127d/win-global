// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.kirigami as Kirigami
import "../details" as Details
Item {
    id:summary
    required property string layoutMode
    required property var entries
    signal connectionRequested(int row)
    signal configureRequested()
    signal refreshRequested()
    signal vpnToggleRequested(int row)
    property bool internetAccess:false
    property bool portal:false
    property string emptyMessage:qsTr("No available networks")
    property bool allNetworks:false
    property string searchText:""
    property real searchHeight:48
    property alias searchHost:availableSearchSlot
    readonly property var available:entries.filter(e=>!e.active && !e.connecting && e.type!==PlasmaNM.Enums.Vpn && e.name.toLocaleLowerCase().includes(searchText.toLocaleLowerCase()))
    readonly property var vpns:entries.filter(e=>e.type===PlasmaNM.Enums.Vpn)
    readonly property var secondary:connected.filter(e=>e!==primary && e.type!==PlasmaNM.Enums.Vpn)
    function kind(record) {return record?.type===PlasmaNM.Enums.Wired?qsTr("Ethernet"):record?.type===PlasmaNM.Enums.Wireless?qsTr("Wi-Fi"):qsTr("Network");}
    function signalText(record) {return record.signal>=70?qsTr("Strong signal"):record.signal>=40?qsTr("Good signal"):qsTr("Weak signal");}
    function subtitle(record) {return record?.connecting?qsTr("Connecting…"):kind(record)+" · "+(portal?qsTr("Sign-in required"):internetAccess?qsTr("Internet access"):qsTr("Connected"));}
    readonly property var connected:entries.filter(e=>e.active || e.connecting)
    readonly property var primary:connected.find(e=>e.type!==PlasmaNM.Enums.Vpn) || connected[0] || null
    readonly property var wireless:entries.find(e=>e.type===PlasmaNM.Enums.Wireless && (e.active || e.connecting)) || null
    readonly property var wired:entries.find(e=>e.type===PlasmaNM.Enums.Wired && (e.active || e.connecting)) || null
    visible:layoutMode!=="compact" && layoutMode!=="details"
    implicitHeight:visible ? body.implicitHeight+8 : 0
    height:implicitHeight
    ColumnLayout {
    id:body;width:parent.width;spacing:10
    Details.DetailSurface {
        objectName:"networkSpotlightCard";Layout.fillWidth:true;radius:24;visible:summary.layoutMode==="spotlight"
        implicitHeight:hero.implicitHeight+32
        ColumnLayout {
            id:hero;anchors.fill:parent;anchors.margins:16;spacing:14
            RowLayout {
                Layout.fillWidth:true
                Rectangle {
                    implicitWidth:tag.implicitWidth+18;implicitHeight:tag.implicitHeight+8;radius:height/2;color:Qt.alpha(Kirigami.Theme.highlightColor,0.16)
                    PC.Label {id:tag;anchors.centerIn:parent;text:qsTr("Primary connection");font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85}
                }
                Item {Layout.fillWidth:true}
                NetworkActionButton {objectName:"networkPrimaryDetails";visible:!!summary.primary;flat:true;display:PC.AbstractButton.IconOnly;icon.name:"view-more-symbolic";text:qsTr("Connection details");Accessible.name:text;onClicked:summary.connectionRequested(summary.primary.row);PC.ToolTip{text:parent.text}}
            }
            RowLayout {
                Layout.fillWidth:true;spacing:12
                Rectangle {
                    Layout.preferredWidth:56;Layout.preferredHeight:56;radius:20;color:Qt.alpha(Kirigami.Theme.highlightColor,0.16)
                    Kirigami.Icon {anchors.centerIn:parent;width:28;height:28;source:summary.primary?.icon || "network-disconnect-symbolic"}
                }
                ColumnLayout {
                    Layout.fillWidth:true;spacing:3
                    PC.Label {Layout.fillWidth:true;text:summary.primary?.name || qsTr("Not connected");textFormat:Text.PlainText;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideRight;font.pointSize:Kirigami.Theme.defaultFont.pointSize*1.3;font.weight:Font.DemiBold}
                    PC.Label {Layout.fillWidth:true;text:summary.primary?summary.subtitle(summary.primary):qsTr("Choose a network to get online");wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85}
                }
            }
            RowLayout {
                Layout.fillWidth:true
                PC.Label {visible:summary.primary?.type===PlasmaNM.Enums.Wireless;Layout.fillWidth:true;wrapMode:Text.Wrap;text:(summary.primary?.secured?qsTr("Secured"):qsTr("Open network"))+" · "+summary.signalText(summary.primary || {});color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85}
                NetworkActionButton {text:summary.primary?.type===PlasmaNM.Enums.Wireless?qsTr("Details"):qsTr("Connection settings");font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85;onClicked:summary.primary?.type===PlasmaNM.Enums.Wireless?summary.connectionRequested(summary.primary.row):summary.configureRequested()}
                Item{Layout.fillWidth:true;visible:summary.primary?.type!==PlasmaNM.Enums.Wireless}
            }
            Repeater {
                model:summary.secondary
                delegate:NetworkActionButton {
                    required property var modelData
                    objectName:"networkSecondary:"+modelData.row
                    Layout.fillWidth:true;Layout.minimumWidth:0;flat:true;text:qsTr("%1 also connected · %2").arg(summary.kind(modelData)).arg(modelData.name);font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85
                    contentItem:PC.Label{text:parent.text;font:parent.font;color:Kirigami.Theme.disabledTextColor;elide:Text.ElideRight}
                    onClicked:summary.connectionRequested(modelData.row)
                    PC.ToolTip{text:parent.text}
                }
            }
        }
    }
    GridLayout {
        Layout.fillWidth:true;visible:summary.layoutMode==="desk";columns:width<330?1:2;columnSpacing:10;rowSpacing:10
        Repeater {
            model:summary.layoutMode==="desk"?2:0
            delegate:Details.DetailSurface {
                required property int index
                readonly property var record:index===0?summary.wireless:summary.wired
                Layout.fillWidth:true;Layout.preferredWidth:1;Layout.minimumWidth:0;radius:24;implicitHeight:card.implicitHeight+24
                ColumnLayout {
                    id:card;anchors.fill:parent;anchors.margins:12;spacing:8
                    PC.Label{text:index===0?qsTr("Wi-Fi"):qsTr("Ethernet");font.weight:Font.DemiBold}
                    Kirigami.Icon{Layout.alignment:Qt.AlignHCenter;Layout.preferredWidth:32;Layout.preferredHeight:32;source:record?.icon || (index===0?"network-wireless-symbolic":"network-wired-symbolic")}
                    PC.Label{Layout.fillWidth:true;text:record?.name || qsTr("Not connected");textFormat:Text.PlainText;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideRight}
                    NetworkActionButton{Layout.fillWidth:true;Layout.minimumWidth:0;flat:true;text:qsTr("Details");enabled:!!record;onClicked:summary.connectionRequested(record.row)}
                }
            }
        }
    }
    Repeater {
        model:summary.layoutMode==="spotlight"?Math.max(1,summary.vpns.length):summary.vpns.length
        delegate:Details.DetailSurface {
            required property int index
            readonly property var record:summary.vpns[index] || null
            objectName:"networkVpnCard";Layout.fillWidth:true;radius:24;implicitHeight:vpnBody.implicitHeight+24
            RowLayout {
                id:vpnBody;anchors.fill:parent;anchors.margins:12;spacing:10
                Rectangle {Layout.preferredWidth:32;Layout.preferredHeight:32;radius:16;color:Qt.alpha(Kirigami.Theme.highlightColor,0.16);Kirigami.Icon{anchors.centerIn:parent;width:18;height:18;source:"network-vpn-symbolic"}}
                ColumnLayout {
                    Layout.fillWidth:true;spacing:2
                    PC.Label{Layout.fillWidth:true;text:record?.name || qsTr("VPN");textFormat:Text.PlainText;elide:Text.ElideRight;font.weight:Font.DemiBold}
                    PC.Label{Layout.fillWidth:true;text:record?(record.connecting?qsTr("Connecting…"):record.active?qsTr("Connected"):qsTr("Not connected")):qsTr("No VPN configured");elide:Text.ElideRight;color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85}
                }
                NetworkActionButton {visible:!record;flat:true;text:qsTr("Set up");onClicked:summary.configureRequested()}
                PC.Switch {visible:!!record;checked:!!record?.active;enabled:!!record && !record.connecting;Accessible.name:record?.name || qsTr("VPN");onToggled:{summary.vpnToggleRequested(record.row);checked=Qt.binding(()=>!!record?.active);}}
                NetworkActionButton {visible:!!record;flat:true;display:PC.AbstractButton.IconOnly;icon.name:"view-more-symbolic";text:qsTr("VPN details");Accessible.name:text;onClicked:summary.connectionRequested(record.row)}
            }
        }
    }
    Details.DetailSurface {
        objectName:"networkAvailableCard";Layout.fillWidth:true;visible:summary.layoutMode==="spotlight";radius:24;implicitHeight:availableBody.implicitHeight+24
        ColumnLayout {
            id:availableBody;anchors.fill:parent;anchors.margins:12;spacing:3
            Item {id:availableSearchSlot;Layout.fillWidth:true;implicitHeight:summary.searchHeight;Layout.bottomMargin:8}
            RowLayout {
                Layout.fillWidth:true;spacing:2
                PC.Label {Layout.fillWidth:true;Layout.leftMargin:4;text:qsTr("Available networks");font.weight:Font.DemiBold}
                NetworkActionButton {flat:true;display:PC.AbstractButton.IconOnly;icon.name:"view-refresh-symbolic";text:qsTr("Refresh networks");Accessible.name:text;onClicked:summary.refreshRequested();PC.ToolTip{text:parent.text}}
            }
            Repeater {
                model:summary.allNetworks?summary.available:summary.available.slice(0,3)
                delegate:NetworkConnectionRow {
                    required property var modelData
                    Layout.fillWidth:true;record:modelData
                    onClicked:summary.connectionRequested(modelData.row)
                }
            }
            PC.Label {visible:summary.available.length===0;Layout.fillWidth:true;Layout.margins:8;text:summary.emptyMessage;wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor}
            NetworkActionButton {objectName:"networkShowAll";Layout.fillWidth:true;flat:true;visible:summary.available.length>3;text:summary.allNetworks?qsTr("Show fewer networks"):qsTr("Show all %1 networks").arg(summary.available.length);icon.name:summary.allNetworks?"arrow-up-symbolic":"arrow-down-symbolic";onClicked:summary.allNetworks=!summary.allNetworks}
        }
    }
    }
}
