// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import "../details" as Details

Item {
    id:island
    required property var controller
    required property string layoutMode
    signal layoutSelected(string mode)
    implicitHeight: actions.implicitHeight + 8
    implicitWidth: actions.implicitWidth + 8
    Details.DetailSurface {
        objectName: "networkActionIsland"
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(island.width, island.implicitWidth)
        height: island.height
        radius: height / 2
        elevated: true
        RowLayout {
            id:actions;anchors.fill:parent;anchors.margins:4;spacing:2
            NetworkActionButton {objectName:"networkSettings";icon.name:"configure-symbolic";display:island.width>370?PC.AbstractButton.TextBesideIcon:PC.AbstractButton.TextOnly;leftPadding:8;rightPadding:8;Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:implicitWidth;flat:true;text:qsTr("Settings");enabled:controller.settingsAllowed;onClicked:controller.settings()}
            NetworkActionButton {objectName:"networkHotspot";icon.name:"network-wireless-hotspot";display:island.width>370?PC.AbstractButton.TextBesideIcon:PC.AbstractButton.TextOnly;leftPadding:8;rightPadding:8;Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:implicitWidth;flat:true;text:qsTr("Hotspot");visible:controller.hotspotAvailable;enabled:controller.hotspotSupported;checkable:true;checked:controller.hotspotActive;onClicked:{controller.toggleHotspot();checked=Qt.binding(()=>controller.hotspotActive);} PC.ToolTip{text:controller.hotspotHint}}
            NetworkActionButton {objectName:"networkScanQr";icon.name:"view-barcode-qr";display:island.width>370?PC.AbstractButton.TextBesideIcon:PC.AbstractButton.TextOnly;leftPadding:8;rightPadding:8;Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:implicitWidth;flat:true;text:qsTr("Scan QR");enabled:controller.qrAvailable;onClicked:controller.scanQr();Accessible.description:controller.qrAvailable?qsTr("Scan a Wi-Fi QR code"):qsTr("Install QRCA Barcode Scanner to scan Wi-Fi QR codes");PC.ToolTip{text:parent.Accessible.description}}
            NetworkActionButton {objectName:"networkLayoutButton";flat:true;display:PC.AbstractButton.IconOnly;icon.name:"view-more-symbolic";text:qsTr("Layouts and more");Accessible.name:text;onClicked:layoutOptions.popup(this,0,-layoutOptions.height);PC.ToolTip{text:parent.text}}
        }
    }
    NetworkMenu {
        id:layoutOptions;objectName:"networkLayoutMenu"
        NetworkMenuItem {text:qsTr("Layout");section:true}
        NetworkMenuItem {objectName:"networkLayoutSpotlight";text:qsTr("Connection spotlight");checkable:true;checked:island.layoutMode==="spotlight";onTriggered:island.layoutSelected("spotlight")}
        NetworkMenuItem {objectName:"networkLayoutDesk";text:qsTr("Connection desk");checkable:true;checked:island.layoutMode==="desk";onTriggered:island.layoutSelected("desk")}
        NetworkMenuItem {objectName:"networkLayoutCompact";text:qsTr("Compact connections");checkable:true;checked:island.layoutMode==="compact";onTriggered:island.layoutSelected("compact")}
        NetworkMenuItem {text:qsTr("Connections");section:true}
        NetworkMenuItem {text:qsTr("Refresh networks");icon.name:"view-refresh-symbolic";onTriggered:controller.scan()}
        NetworkMenuItem {text:qsTr("Saved / hidden networks…");icon.name:"configure-symbolic";enabled:controller.settingsAllowed;onTriggered:controller.settings()}
    }
}
