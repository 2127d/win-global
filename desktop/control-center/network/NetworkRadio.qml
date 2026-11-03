// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details
Details.DetailSurface {
    id:card
    required property string title
    required property string iconName
    required property bool checked
    property bool busy:false
    signal toggled()
    radius:height/2
    implicitHeight:body.implicitHeight+20
    RowLayout {
        id:body;anchors.fill:parent;anchors.margins:10;spacing:8
        Kirigami.Icon {visible:card.width>165;source:card.iconName;isMask:true;Layout.preferredWidth:18;Layout.preferredHeight:18}
        ColumnLayout {
            Layout.fillWidth:true;spacing:0
            PC.Label {Layout.fillWidth:true;text:card.title;elide:Text.ElideRight;font.weight:Font.DemiBold;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9}
            PC.Label {text:card.busy?qsTr("Scanning…"):card.checked?qsTr("On"):qsTr("Off");color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.8}
        }
        PC.Switch {
            id:toggle;objectName:card.objectName+"Switch";text:card.title;Accessible.name:card.title
            checked:card.checked;onToggled:{card.toggled();checked=Qt.binding(()=>card.checked);}
            padding:0;implicitWidth:36;implicitHeight:28;contentItem:Item {}
            indicator:Rectangle {
                width:36;height:22;y:(toggle.height-height)/2;radius:height/2
                color:toggle.checked?Kirigami.Theme.highlightColor:Qt.alpha(Kirigami.Theme.textColor,0.18)
                border.width:toggle.visualFocus?2:0;border.color:Kirigami.Theme.focusColor
                Rectangle {width:18;height:18;y:2;x:2+toggle.visualPosition*16;radius:9;color:Kirigami.Theme.backgroundColor}
            }
        }
    }
}
