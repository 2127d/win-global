// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
Item {
    id: hub
    objectName: "appearanceHub"
    property alias trialDialog: confirmation
    property string selectedMaterial: appearancePreview.activeMaterial
    property string selectedMode: appearancePreview.colorMode
    property bool modeEdited: false
    property bool showComponents: true
    readonly property bool wide: width >= 700
    readonly property bool dirty: selectedMaterial !== appearancePreview.activeMaterial || (modeEdited && (selectedMode !== appearancePreview.colorMode || appearancePreview.automaticMode))
    readonly property var componentOrder: ["kcm_lookandfeel", "kcm_style", "kcm_desktoptheme", "kcm_colors", "kcm_kwindecoration", "kcm_icons", "kcm_cursortheme", "kcm_fonts", "kcm_fontinst", "kcm_wallpaper", "kcm_animations", "kcm_soundtheme", "kcm_splashscreen", "kcm_sequoia_taskbar"]
    readonly property var components: {
        const entries = navigation.catalog.filter(e => e.group === "appearance");
        return entries.sort((a,b) => {
            const ai = componentOrder.indexOf(a.id), bi = componentOrder.indexOf(b.id);
            return (ai < 0 ? 100 : ai) - (bi < 0 ? 100 : bi);
        });
    }
    function reset() { selectedMaterial = appearancePreview.activeMaterial; selectedMode = appearancePreview.colorMode; modeEdited = false; presets.currentIndex = 0; }
    function apply() { appearancePreview.applyLook(selectedMaterial, modeEdited ? selectedMode : "current"); }
    property bool operationWasBusy: false
    Connections {
        target:appearancePreview
        function onSurfaceChanged() { if (!appearancePreview.busy) hub.reset(); }
        function onChanged() {
            if (hub.operationWasBusy && !appearancePreview.busy) hub.reset();
            hub.operationWasBusy=appearancePreview.busy;
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                K.Heading { text: i18n("Appearance"); level: 1; font.weight: Font.DemiBold }
                C.Label { text: i18n("One look, all your desktop surfaces"); opacity: .75; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            }
            C.Button { text: hub.width > 620 ? i18n("Save look…") : ""; icon.name: "document-save"; Accessible.name: i18n("Save look"); onClicked: saveDialog.open() }
            C.Button { text: hub.width > 620 ? i18n("Get new themes…") : ""; icon.name: "get-hot-new-stuff"; Accessible.name: i18n("Get new themes"); onClicked: navigation.open("kcm_lookandfeel"); C.ToolTip.visible: hovered; C.ToolTip.text: i18n("Browse and download themes in KDE Global Theme") }
            C.ToolButton { text: "⋯"; Accessible.name: i18n("More appearance options"); onClicked: moreMenu.popup()
                C.Menu { id: moreMenu
                    C.MenuItem { text: hub.showComponents ? i18n("Hide appearance components") : i18n("Show appearance components"); onTriggered: hub.showComponents = !hub.showComponents }
                    C.MenuItem { text: i18n("Undo last applied look"); enabled: !appearancePreview.busy; onTriggered: appearancePreview.trial("restore") }
                    C.MenuItem { text: i18n("Global Theme…"); onTriggered: navigation.open("kcm_lookandfeel") }
                }
            }
        }
        GridLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            columns: hub.wide && hub.showComponents ? 2 : 1
            columnSpacing: 14; rowSpacing: 10
            C.ScrollView {
                id: bodyScroll
                objectName: "appearanceMainContent"
                Layout.fillWidth: true; Layout.fillHeight: true
                Layout.minimumWidth: 0
                contentWidth: availableWidth
                // Aspect-ratio previews change height with viewport width. Reserve
                // the scrollbar gutter even while hidden so overflow cannot toggle
                // the width and repeatedly add/remove the scrollbar.
                leftPadding: (background?.visible ? (background.leftPadding ?? 0) : 0)
                             + (mirrored ? C.ScrollBar.vertical.implicitWidth : 0)
                rightPadding: (background?.visible ? (background.rightPadding ?? 0) : 0)
                              + (mirrored ? 0 : C.ScrollBar.vertical.implicitWidth)
                C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
                ColumnLayout {
                    width: bodyScroll.availableWidth; spacing: 12
                    GridLayout {
                        Layout.fillWidth: true; columns: width > 460 ? 2 : 1
                        AppearanceThemePill {
                            id: presets; objectName: "appearancePresets"
                            backdropItem:hub
                            Layout.fillWidth: true; Layout.minimumWidth: 0
                            textRole: "name"
                            model: [{name:i18n("Sequoia · Custom")}].concat(appearancePreview.savedLooks).concat([{name:i18n("Choose global theme…"), browse:true}])
                            onActivated: {
                                const choice = model[currentIndex];
                                if (choice.browse) { currentIndex = 0; navigation.open("kcm_lookandfeel"); }
                                else if (choice.material) { hub.selectedMaterial = choice.material; hub.selectedMode = choice.mode; hub.modeEdited = true; }
                                else hub.reset();
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            AppearanceModePill {
                                Layout.preferredWidth:210; Layout.minimumWidth:150
                                mode:appearancePreview.automaticMode && !hub.modeEdited ? "auto" : hub.selectedMode
                                onModeSelected:function(mode) {
                                    if (mode === "auto") navigation.open("kcm_lookandfeel");
                                    else { hub.selectedMode=mode; hub.modeEdited=true; }
                                }
                            }
                            C.AbstractButton {
                                id: schedule
                                objectName: "appearanceScheduleButton"
                                text:i18n("Schedule…")
                                implicitHeight:42; implicitWidth:contentItem.implicitWidth+32
                                leftPadding:16; rightPadding:16; hoverEnabled:true
                                Layout.minimumWidth:implicitWidth
                                contentItem:C.Label {
                                    text:schedule.text; font:schedule.font; color:K.Theme.textColor
                                    horizontalAlignment:Text.AlignHCenter; verticalAlignment:Text.AlignVCenter
                                }
                                background:HubPanel {
                                    radius:height/2; tint:schedule.down ? .75 : schedule.hovered ? .6 : .35
                                    emphasized:schedule.visualFocus
                                    Behavior on color { ColorAnimation { duration:120 } }
                                }
                                onClicked:navigation.open("kcm_nighttime")
                            }
                        }
                    }
                    GridLayout {
                        Layout.fillWidth:true; columns: width >= 380 ? 3 : 1; uniformCellWidths:true; columnSpacing:10; rowSpacing:10
                        Repeater {
                            model: [{id:"solid",name:i18n("Solid")},{id:"transparent",name:i18n("Transparent")},{id:"glass",name:i18n("Liquid Glass")}]
                            C.AbstractButton {
                                id: materialButton
                                required property var modelData
                                objectName: "material-" + modelData.id
                                Layout.fillWidth:true; Layout.minimumWidth:0
                                implicitHeight:contentItem.implicitHeight + topPadding + bottomPadding; padding:7; hoverEnabled:true
                                Accessible.name:modelData.name; Accessible.role:Accessible.RadioButton; Accessible.checked:hub.selectedMaterial===modelData.id
                                onClicked:hub.selectedMaterial=modelData.id
                                background:HubPanel { radius:16; emphasized:materialButton.activeFocus || hub.selectedMaterial===materialButton.modelData.id; tint:materialButton.hovered ? .6 : .3 }
                                contentItem:ColumnLayout {
                                    DesktopPreview { Layout.fillWidth:true; Layout.preferredHeight:width/desktopRenderer.aspectRatio; miniature:true; material:materialButton.modelData.id; colorMode:hub.modeEdited ? hub.selectedMode : "current" }
                                    RowLayout { Layout.fillWidth:true
                                        C.Label { Layout.fillWidth:true; text:materialButton.modelData.name; font.weight:Font.DemiBold; horizontalAlignment:Text.AlignHCenter; elide:Text.ElideRight }
                                        K.Icon { visible:hub.selectedMaterial===materialButton.modelData.id; source:"emblem-checked"; implicitWidth:16; implicitHeight:16 }
                                    }
                                }
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth:true
                        C.Label { Layout.fillWidth:true; text:i18n("Desktop preview"); font.weight:Font.DemiBold }
                        C.ToolButton { text:i18n("Refresh"); icon.name:"view-refresh"; onClicked:desktopRenderer.refresh() }
                    }
                    HubPanel {
                        Layout.fillWidth:true; implicitHeight:(width-16)/desktopRenderer.aspectRatio+16; radius:22
                        DesktopPreview { anchors.fill:parent; anchors.margins:8; material:hub.selectedMaterial; colorMode:hub.modeEdited ? hub.selectedMode : "current" }
                    }
                    HubPanel {
                        Layout.fillWidth:true; implicitHeight:keepOptions.implicitHeight+12
                        Flow { id:keepOptions; anchors.left:parent.left; anchors.right:parent.right; anchors.top:parent.top; anchors.margins:6; spacing:8
                            C.CheckBox { text:i18n("Keep my wallpaper"); checked:true; enabled:false; palette.disabled.windowText:K.Theme.textColor; palette.disabled.text:K.Theme.textColor }
                            C.CheckBox { text:i18n("Keep panel & widget layout"); checked:true; enabled:false; palette.disabled.windowText:K.Theme.textColor; palette.disabled.text:K.Theme.textColor }
                        }
                        C.ToolTip.visible: keepHover.hovered
                        C.ToolTip.text:i18n("Material and color changes always preserve these. Use Global Theme to replace wallpaper or desktop layout.")
                        HoverHandler { id:keepHover }
                    }
                    C.ItemDelegate {
                        Layout.fillWidth:true; text:i18n("App appearance & compatibility"); icon.name:"preferences-desktop-theme-global"; implicitHeight:52
                        background:HubPanel { tint:.30; emphasized:parent.activeFocus }
                        onClicked:compatibility.open()
                    }
                    C.Label { Layout.fillWidth:true; text:i18n("Your desktop layout with a Files window · Wallpaper and layout are preserved when you apply."); wrapMode:Text.WordWrap; opacity:.65; font.pointSize:K.Theme.smallFont.pointSize }
                }
            }
            HubPanel {
                id: componentPanel
                objectName: "appearanceComponentsPanel"
                visible: hub.showComponents
                Layout.fillHeight:hub.wide; Layout.fillWidth:!hub.wide
                Layout.preferredWidth:hub.wide ? 260 : -1
                Layout.preferredHeight:hub.wide ? -1 : 220
                Layout.minimumWidth:0
                tint:.42
                ColumnLayout {
                    anchors.fill:parent; anchors.margins:12; spacing:6
                    RowLayout {
                        Layout.fillWidth:true
                        C.Label { Layout.fillWidth:true; text:i18n("Appearance components"); font.weight:Font.DemiBold; wrapMode:Text.WordWrap }
                        C.ToolButton { icon.name:"window-close"; Accessible.name:i18n("Hide appearance components"); onClicked:hub.showComponents=false }
                    }
                    C.ScrollView {
                        Layout.fillWidth:true; Layout.fillHeight:true; contentWidth:availableWidth
                        C.ScrollBar.horizontal.policy:C.ScrollBar.AlwaysOff
                        ColumnLayout {
                            width:parent.width; spacing:2
                            Repeater {
                                model:hub.components
                                C.ItemDelegate {
                                    id: componentRow
                                    required property var modelData
                                    objectName:"appearanceComponent-"+modelData.id
                                    Layout.fillWidth:true; Layout.minimumWidth:0; implicitHeight:37
                                    text:modelData.name
                                    contentItem:RowLayout {
                                        K.Icon { source:componentRow.modelData.icon; implicitWidth:21; implicitHeight:21 }
                                        C.Label { text:componentRow.modelData.name; Layout.fillWidth:true; elide:Text.ElideRight }
                                        K.Icon { source:"go-next"; implicitWidth:14; implicitHeight:14; opacity:.6 }
                                    }
                                    onClicked:navigation.open(modelData.id)
                                }
                            }
                        }
                    }
                    C.Button { Layout.fillWidth:true; text:i18n("Related settings"); icon.name:"insert-link"; onClicked:relatedMenu.popup()
                        C.Menu { id:relatedMenu
                            C.MenuItem { text:i18n("Desktop Effects"); onTriggered:navigation.open("kcm_kwin_effects") }
                            C.MenuItem { text:i18n("Task Switcher"); onTriggered:navigation.open("kcm_kwintabbox") }
                            C.MenuItem { text:i18n("Screen Locking"); onTriggered:navigation.open("kcm_screenlocker") }
                            C.MenuItem { text:i18n("Login Screen"); onTriggered:navigation.open("kcm_plasmalogin") }
                        }
                    }
                }
            }
        }
        C.Label { visible:appearancePreview.status !== i18n("Preview only. Your desktop is unchanged."); Layout.fillWidth:true; text:appearancePreview.status; font.pointSize:K.Theme.smallFont.pointSize; wrapMode:Text.WordWrap; Accessible.role:Accessible.AlertMessage }
        GridLayout {
            objectName:"appearanceFooter"
            Layout.fillWidth:true; columns:hub.width>600 ? 2 : 1; columnSpacing:12; rowSpacing:6
            RowLayout {
            Layout.fillWidth:true; spacing:6
            C.Button { text:i18n("Help"); icon.name:"help-contextual"; onClicked:helpDialog.open() }
            C.Button { text:i18n("Defaults"); icon.name:"edit-undo"; enabled:!appearancePreview.busy; onClicked:{ hub.selectedMaterial="glass"; hub.selectedMode=appearancePreview.colorMode; hub.modeEdited=false; } }
            C.Button { objectName:"appearanceResetButton"; text:i18n("Reset"); icon.name:"edit-reset"; enabled:hub.dirty && !appearancePreview.busy; onClicked:hub.reset() }
            Item { Layout.fillWidth:true }
            }
            RowLayout { Layout.alignment:Qt.AlignRight; spacing:6
            C.Button { objectName:"desktopTrialButton"; text:i18n("Preview desktop"); icon.name:"video-display"; visible:!appearancePreview.busy; onClicked:confirmation.open() }
            C.Button { objectName:"appearanceApplyButton"; text:i18n("Apply"); icon.name:"dialog-ok-apply"; enabled:hub.dirty && !appearancePreview.busy; visible:!appearancePreview.busy; highlighted:enabled; onClicked:hub.apply() }
            C.Button { text:i18n("Revert (%1)",appearancePreview.seconds); visible:appearancePreview.busy; onClicked:appearancePreview.revert() }
            C.Button { text:i18n("Keep"); visible:appearancePreview.busy; enabled:appearancePreview.seconds>0; onClicked:appearancePreview.keep() }
            }
        }
    }
    C.Dialog {
        id:confirmation; objectName:"materialTrialConfirmation"; parent:C.Overlay.overlay; anchors.centerIn:parent
        width:Math.min(480,parent.width-32); modal:true; title:i18n("Preview this desktop look?"); standardButtons:C.Dialog.Ok|C.Dialog.Cancel
        onAccepted:appearancePreview.previewLook(hub.selectedMaterial,hub.modeEdited ? hub.selectedMode : "current")
        contentItem:C.Label { wrapMode:Text.WordWrap; text:i18n("Preview the material and color mode for 15 seconds. Choose Keep to retain them, or they restore automatically. Apply keeps your selection directly; Undo last applied look is available in the more menu.") }
    }
    C.Dialog {
        id:saveDialog; parent:C.Overlay.overlay; anchors.centerIn:parent; width:Math.min(440,parent.width-32); modal:true; title:i18n("Save look"); standardButtons:C.Dialog.Save|C.Dialog.Cancel
        onOpened: standardButton(C.Dialog.Save).enabled = Qt.binding(function() { return lookName.text.trim().length > 0; })
        onAccepted: { if (lookName.text.trim().length) appearancePreview.saveLook(lookName.text,hub.selectedMaterial,hub.selectedMode); }
        contentItem:ColumnLayout {
            C.TextField { id:lookName; Layout.fillWidth:true; placeholderText:i18n("Look name"); maximumLength:80 }
            C.Label { Layout.fillWidth:true; text:i18n("Saves this Sequoia material and light/dark color mode as a reusable preset. Individual components remain managed in KDE settings."); wrapMode:Text.WordWrap }
        }
    }
    C.Dialog {
        id:compatibility; parent:C.Overlay.overlay; anchors.centerIn:parent; width:Math.min(460,parent.width-32); modal:true; title:i18n("App appearance & compatibility"); standardButtons:C.Dialog.Close
        contentItem:ColumnLayout {
            C.Label { Layout.fillWidth:true; wrapMode:Text.WordWrap; text:i18n("KDE apps follow the selected material. GTK, Electron, and custom app surfaces can use their own appearance. Some apps need reopening after a style change.") }
            C.Button { text:i18n("Application Style & GTK settings…"); onClicked:{ compatibility.close(); navigation.open("kcm_style"); } }
            C.Button { text:i18n("Window Rules & app overrides…"); onClicked:{ compatibility.close(); navigation.open("kcm_kwinrules"); } }
        }
    }
    C.Dialog {
        id:helpDialog; parent:C.Overlay.overlay; anchors.centerIn:parent; width:Math.min(460,parent.width-32); modal:true; title:i18n("Appearance help"); standardButtons:C.Dialog.Close
        contentItem:C.Label { wrapMode:Text.WordWrap; text:i18n("Choose a material and optional light/dark color mode. Apply keeps the selected look. Preview desktop offers a 15-second trial. Reset discards un-applied choices; Defaults selects Liquid Glass. Wallpaper and desktop layout are always preserved here. Auto and Schedule open KDE’s native automatic theme and day-night controls.") }
    }
}
