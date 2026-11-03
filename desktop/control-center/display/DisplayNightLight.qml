// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    id:card
    required property var control
    property bool settingsAllowed:true
    signal configureRequested()
    signal toggleRequested()
    radius:24;implicitHeight:body.implicitHeight+28
    readonly property string statusText: {
        if(control.inhibited && control.enabled)return qsTr("Suspended");
        if(!control.available)return qsTr("Unavailable");
        if(!control.enabled)return qsTr("Not enabled");
        if(!control.running)return qsTr("Not running");
        if(!control.hasSwitchingTimes)return qsTr("On");
        return control.daylight ? (control.transitioning?qsTr("Morning transition"):qsTr("Day")) : (control.transitioning?qsTr("Evening transition"):qsTr("Night"));
    }
    ColumnLayout {
        id:body;anchors.fill:parent;anchors.margins:14;spacing:8
        RowLayout {
            Layout.fillWidth:true;spacing:10
            Rectangle {
                Layout.preferredWidth:34;Layout.preferredHeight:34;radius:17;color:Qt.alpha(Kirigami.Theme.highlightColor,0.16)
                Kirigami.Icon {anchors.centerIn:parent;width:20;height:20;source:"redshift-status-on";isMask:true}
            }
            ColumnLayout {
                Layout.fillWidth:true;spacing:2
                PC.Label{text:qsTr("Night Light");font.weight:Font.DemiBold}
                PC.Label{Layout.fillWidth:true;text:card.statusText;wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor}
            }
            PC.Label{visible:control.available && control.enabled && control.running;text:qsTr("%1 K").arg(control.currentTemperature);color:Kirigami.Theme.disabledTextColor}
        }
        RowLayout {
            Layout.fillWidth:true;spacing:6
            PC.Switch {objectName:"displayNightSuspend";visible:control.enabled;enabled:control.togglable;checked:control.inhibited;text:qsTr("Suspend");Accessible.name:qsTr("Suspend Night Light");onToggled:card.toggleRequested()}
            Item {Layout.fillWidth:true;visible:!control.enabled}
            DisplayActionButton {objectName:"displayNightConfigure";Layout.fillWidth:control.enabled;Layout.minimumWidth:0;visible:card.settingsAllowed;icon.name:"configure-symbolic";text:control.enabled?qsTr("Configure…"):qsTr("Enable and configure…");onClicked:card.configureRequested()}
        }
        PC.Label {
            Layout.fillWidth:true;visible:control.running && control.hasSwitchingTimes
            text: {
                const time=new Date(control.transitioning?control.currentTransitionEndTime:control.scheduledTransitionStartTime).toLocaleTimeString([],{hour:"2-digit",minute:"2-digit"});
                if(control.transitioning)return control.daylight?qsTr("Transition to day completes at %1").arg(time):qsTr("Transition to night completes at %1").arg(time);
                return control.daylight?qsTr("Transition to night scheduled for %1").arg(time):qsTr("Transition to day scheduled for %1").arg(time);
            }
            wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9
        }
    }
}
