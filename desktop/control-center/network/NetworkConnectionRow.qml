// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details
NetworkActionButton {
    id:row
    required property var record
    objectName:"networkAvailable:"+record.row
    flat:true;implicitHeight:Math.max(56,implicitContentHeight+16);leftPadding:8;rightPadding:8
    Accessible.name:record.name
    readonly property string securityLabel:record.saved?qsTr("Saved"):record.securityKnown===false?"":record.security || (record.secured?qsTr("Secured"):qsTr("Open network"))
    readonly property string signalLabel:record.signal?(record.signal>=70?qsTr("Strong signal"):record.signal>=40?qsTr("Good signal"):qsTr("Weak signal")):""
    contentItem:RowLayout {
        spacing:10
        Kirigami.Icon{source:record.type===PlasmaNM.Enums.Wireless?"network-wireless-symbolic":record.icon;Layout.preferredWidth:22;Layout.preferredHeight:22}
        ColumnLayout {
            Layout.fillWidth:true;spacing:2
            PC.Label{Layout.fillWidth:true;text:record.name;textFormat:Text.PlainText;elide:Text.ElideRight;font.weight:Font.Medium}
            PC.Label{Layout.fillWidth:true;text:[row.securityLabel,row.signalLabel].filter(part=>!!part).join(" · ");elide:Text.ElideRight;color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.85}
        }
        Kirigami.Icon{source:record.secured?"object-locked-symbolic":"go-next-symbolic";Layout.preferredWidth:16;Layout.preferredHeight:16;isMask:true;color:Kirigami.Theme.disabledTextColor}
    }
}
