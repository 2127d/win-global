// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    required property var controller
    property Component nightLightCard
    property Component keyboardColorCard
    property string selectedId: ""
    property bool allExpanded: false
    readonly property var displays: controller.displays
    readonly property var selected: displays.find(d=>d.id===selectedId) || displays[0] || null
    objectName: "displaySpotlightView"
    function reconcileSelection() {
        if (!displays.some(d=>d.id===selectedId)) selectedId=displays[0]?.id || "";
        if (displays.length<2) allExpanded=false;
    }
    onDisplaysChanged: reconcileSelection()
    Component.onCompleted: reconcileSelection()
    function select(id, collapse) {
        if (!displays.some(d=>d.id===id)) return;
        selectedId=id;
        if (collapse) {allExpanded=false;scroll.contentItem.contentY=0;}
    }
    function percent(record) { return record?.maximum>0 ? qsTr("%1%").arg(Math.round(100*record.brightness/record.maximum)) : qsTr("Unavailable"); }
    PC.ScrollView {
        id: scroll
        objectName: "displayScroll"
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: -6; anchors.rightMargin: -6
        anchors.bottom: island.top; anchors.bottomMargin: -8
        background: null
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect { maskEnabled: true; maskSource: edgeMask; maskThresholdMin: 0.5; maskSpreadAtMin: 1; autoPaddingEnabled: false }
        contentItem: Flickable {
            clip: true; contentWidth: width; contentHeight: content.implicitHeight+40
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content
                x: 6; y: 6; width: Math.max(0,scroll.availableWidth-12); spacing: 12
                Details.DetailSurface {
                    objectName: "displayFeaturedCard"
                    Layout.fillWidth: true; visible: !!view.selected
                    radius: 24; implicitHeight: featured.implicitHeight+28
                    ColumnLayout {
                        id: featured
                        anchors.fill: parent; anchors.margins: 14; spacing: 12
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                implicitWidth: selectedLabel.implicitWidth+18; implicitHeight: selectedLabel.implicitHeight+8
                                radius: height/2; color: Qt.alpha(Kirigami.Theme.highlightColor,0.16)
                                PC.Label {id:selectedLabel;anchors.centerIn:parent;text:qsTr("Selected display");font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9}
                            }
                            Item { Layout.fillWidth:true }
                            PC.Label { text:qsTr("%1 connected").arg(view.displays.length);color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9 }
                        }
                        RowLayout {
                            objectName: "displayNumberSelector"
                            Layout.fillWidth: true; spacing: 6
                            visible: view.displays.length>1 && view.displays.length<=5
                            Repeater {
                                model: view.displays.length<=5 ? view.displays.length : 0
                                delegate: DisplayActionButton {
                                    required property int index
                                    readonly property var record: view.displays[index] || {}
                                    objectName: "displaySelect:"+record.id
                                    Layout.fillWidth: true; Layout.preferredWidth:1; Layout.minimumWidth:0
                                    text: String(record.number || "");checkable:true;checked:record.id===view.selectedId
                                    Accessible.name: qsTr("Display %1: %2").arg(record.number).arg(record.name)
                                    onClicked: view.select(record.id,false)
                                    PC.ToolTip { text:parent.Accessible.name }
                                }
                            }
                        }
                        DisplayActionButton {
                            objectName: "displayOverflowSelector"
                            Layout.fillWidth:true;visible:view.displays.length>5
                            text:qsTr("Display %1 · %2").arg(view.selected?.number || "").arg(view.selected?.name || "")
                            icon.name:"arrow-down-symbolic"
                            onClicked: displayMenu.popup(this,0,height)
                        }
                        RowLayout {
                            Layout.fillWidth:true;spacing:12
                            Item {
                                Layout.preferredWidth:64;Layout.preferredHeight:52
                                Rectangle {x:0;y:2;width:64;height:42;radius:8;color:Qt.alpha(Kirigami.Theme.highlightColor,0.2);border.width:2;border.color:Qt.alpha(Kirigami.Theme.textColor,0.15)
                                    PC.Label { anchors.centerIn:parent;text:String(view.selected?.number || "");font.weight:Font.DemiBold }
                                }
                                Rectangle {x:21;y:47;width:22;height:3;radius:2;color:Qt.alpha(Kirigami.Theme.textColor,0.2)}
                            }
                            ColumnLayout {
                                Layout.fillWidth:true;spacing:3
                                PC.Label {objectName:"displaySelectedName";Layout.fillWidth:true;text:view.selected?.name || "";textFormat:Text.PlainText;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideRight;font.weight:Font.DemiBold;font.pointSize:Kirigami.Theme.defaultFont.pointSize*1.15}
                                PC.Label {Layout.fillWidth:true;text:qsTr("Display %1").arg(view.selected?.number || "");color:Kirigami.Theme.disabledTextColor}
                            }
                            DisplayActionButton {flat:true;visible:controller.settingsAllowed;display:PC.AbstractButton.IconOnly;icon.name:"view-more-symbolic";text:qsTr("Display settings");Accessible.name:text;onClicked:controller.settings();PC.ToolTip{text:parent.text}}
                        }
                        ColumnLayout {
                            objectName:"displayFeaturedBrightness"
                            Layout.fillWidth:true;visible:!view.allExpanded;spacing:4
                            RowLayout {Layout.fillWidth:true;PC.Label{text:qsTr("Brightness");Layout.fillWidth:true} PC.Label{objectName:"displaySelectedPercent";text:view.percent(view.selected);color:Kirigami.Theme.disabledTextColor}}
                            RowLayout { Layout.fillWidth:true;spacing:10
                                Kirigami.Icon{source:"brightness-high-symbolic";Layout.preferredWidth:18;Layout.preferredHeight:18;isMask:true}
                                DisplayLevelSlider { Layout.fillWidth:true; record:view.selected || {};controller:view.controller }
                            }
                        }
                    }
                }
                Details.DetailSurface {
                    Layout.fillWidth:true;visible:!view.selected;radius:24;implicitHeight:emptyText.implicitHeight+32
                    PC.Label {id:emptyText;anchors.fill:parent;anchors.margins:16;text:qsTr("No displays with brightness control are available.");wrapMode:Text.Wrap}
                }
                Details.DetailSurface {
                    objectName:"displayAllCard"
                    Layout.fillWidth:true;visible:view.displays.length>1;radius:24;implicitHeight:allContent.implicitHeight+16
                    ColumnLayout {
                        id:allContent;anchors.fill:parent;anchors.margins:8;spacing:4
                        DisplayActionButton {
                            objectName:"displayExpandAll"
                            Layout.fillWidth:true;flat:true;text:qsTr("All displays · %1").arg(view.displays.length)
                            icon.name:view.allExpanded?"arrow-up-symbolic":"arrow-down-symbolic";Accessible.description:view.allExpanded?qsTr("Expanded"):qsTr("Collapsed")
                            onClicked:view.allExpanded=!view.allExpanded
                        }
                        Repeater {
                            // Keep delegate identity during live brightness changes and dragging.
                            model:view.allExpanded?view.displays.length:0
                            delegate: ColumnLayout {
                                required property int index
                                readonly property var record:view.displays[index] || {}
                                objectName:"displayRow:"+record.id
                                Layout.fillWidth:true;Layout.leftMargin:6;Layout.rightMargin:6;spacing:2
                                RowLayout {
                                    Layout.fillWidth:true;spacing:6
                                    DisplayActionButton {objectName:"displayRowSelect:"+record.id;Layout.fillWidth:true;Layout.minimumWidth:0;flat:true;text:qsTr("%1 · %2").arg(record.number || "").arg(record.name || "");Accessible.name:text;onClicked:view.select(record.id,true);PC.ToolTip{text:parent.text}}
                                    PC.Label{text:view.percent(record);color:Kirigami.Theme.disabledTextColor}
                                }
                                DisplayLevelSlider {Layout.fillWidth:true;record:parent.record;controller:view.controller}
                            }
                        }
                    }
                }
                Details.DetailSurface {
                    Layout.fillWidth:true;visible:controller.keyboardAvailable;radius:24;implicitHeight:keyboardBody.implicitHeight+28
                    ColumnLayout {
                        id:keyboardBody;anchors.fill:parent;anchors.margins:14;spacing:6
                        RowLayout {Layout.fillWidth:true;Kirigami.Icon{source:"input-keyboard-brightness-symbolic";Layout.preferredWidth:20;Layout.preferredHeight:20;isMask:true} PC.Label{Layout.fillWidth:true;text:qsTr("Keyboard Backlight")} PC.Label{text:view.percent(controller.keyboardRecord);color:Kirigami.Theme.disabledTextColor}}
                        DisplayLevelSlider {Layout.fillWidth:true;record:controller.keyboardRecord;controller:view.controller}
                    }
                }
                Loader { Layout.fillWidth:true;sourceComponent:view.keyboardColorCard;active:controller.keyboardColorAvailable;visible:active }
                Loader { objectName:"displayNightLight";Layout.fillWidth:true;sourceComponent:view.nightLightCard }
                Details.DetailSurface {
                    objectName:"displayThemeCard"
                    Layout.fillWidth:true;radius:24;implicitHeight:themeBody.implicitHeight+28
                    ColumnLayout {
                        id:themeBody;anchors.fill:parent;anchors.margins:14;spacing:8
                        RowLayout {
                            Layout.fillWidth:true;spacing:10
                            Kirigami.Icon{source:"lighttable";Layout.preferredWidth:28;Layout.preferredHeight:28}
                            ColumnLayout {Layout.fillWidth:true;spacing:2;PC.Label{text:qsTr("Theme");font.weight:Font.DemiBold} PC.Label{objectName:"displayCurrentTheme";Layout.fillWidth:true;text:controller.currentTheme;textFormat:Text.PlainText;wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor}}
                            DisplayActionButton{objectName:"displayThemeSettings";visible:controller.themeSettingsAllowed;flat:true;display:PC.AbstractButton.IconOnly;icon.name:"configure-symbolic";text:qsTr("Configure theme");Accessible.name:text;onClicked:controller.themeSettings();PC.ToolTip{text:parent.text}}
                        }
                        RowLayout {Layout.fillWidth:true;PC.Label{Layout.fillWidth:true;text:qsTr("Dark Mode")} PC.Switch{objectName:"displayDarkMode";checked:controller.darkMode;Accessible.name:qsTr("Dark Mode");onToggled:controller.setDarkMode(checked)}}
                        PC.Label{objectName:"displayOtherTheme";Layout.fillWidth:true;text:qsTr("Switch to %1").arg(controller.otherTheme);textFormat:Text.PlainText;wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor;font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9}
                    }
                }
                PC.Label {Layout.fillWidth:true;Layout.leftMargin:8;Layout.rightMargin:8;visible:!!controller.statusMessage;text:controller.statusMessage;wrapMode:Text.Wrap;color:Kirigami.Theme.disabledTextColor}
            }
        }
    }
    DisplayMenu {
        id:displayMenu
        Instantiator {
            model:view.displays.length
            delegate: DisplayMenuItem {
                required property int index
                readonly property var record:view.displays[index] || {}
                objectName:"displayMenuSelect:"+record.id
                text:qsTr("%1 · %2").arg(record.number || "").arg(record.name || "")
                checkable:true;checked:record.id===view.selectedId
                onTriggered:view.select(record.id,false)
            }
            onObjectAdded:(index,object)=>displayMenu.insertItem(index,object)
            onObjectRemoved:(index,object)=>displayMenu.removeItem(object)
        }
    }
    Rectangle {
        id:edgeMask;width:scroll.width;height:scroll.height;visible:false;layer.enabled:true
        readonly property real fade:Math.min(0.12,18/Math.max(1,height))
        gradient:Gradient{GradientStop{position:0;color:scroll.contentItem.contentY>2?"transparent":"white"} GradientStop{position:edgeMask.fade;color:"white"} GradientStop{position:1-edgeMask.fade;color:"white"} GradientStop{position:1;color:"transparent"}}
    }
    Details.DetailSurface {
        id:island;objectName:"displayActionIsland"
        anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom
        radius:24;elevated:true;implicitHeight:footer.implicitHeight+10
        RowLayout {id:footer;anchors.fill:parent;anchors.margins:5;spacing:4
            DisplayActionButton{objectName:"displaySettings";Layout.fillWidth:true;Layout.minimumWidth:0;flat:true;text:qsTr("Display settings");icon.name:"configure-symbolic";enabled:controller.settingsAllowed;onClicked:controller.settings()}
            DisplayActionButton{objectName:"displayIdentify";Layout.minimumWidth:0;flat:true;text:qsTr("Identify");icon.name:"video-display-symbolic";enabled:!!view.selected;onClicked:controller.identify();PC.ToolTip{text:qsTr("Show display names on your screens")}}
        }
    }
}
