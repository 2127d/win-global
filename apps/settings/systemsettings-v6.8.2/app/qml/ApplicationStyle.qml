// SPDX-License-Identifier: GPL-2.0-or-later
// The native style KCM still owns previews, configuration, installs and saves.
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as K
import org.kde.kcmutils as KCM
import org.kde.newstuff as NewStuff
import org.kde.private.kcms.style as Private
import org.sequoia.components 1.0 as S
import "qrc:/kcm/kcm_style" as NativeStyle
import "qrc:/qt/qml/org/kde/systemsettings" as Hub

KCM.SimpleKCM {
    id: root
    objectName: "applicationStyleHub"
    title: ""; globalToolBarStyle: K.ApplicationHeaderStyle.None
    verticalScrollBarPolicy: C.ScrollBar.AlwaysOff
    implicitWidth: 820; implicitHeight: 680
    leftPadding: 14; rightPadding: 14; topPadding: 12; bottomPadding: 12
    property S.Theme theme: Hub.AppearanceLibraryTheme {}
    property var backend: kcm
    property var settings: kcm.styleSettings
    property bool gtkMode: false
    property Hub.ScrollEdgeFade edgeFade: Hub.ScrollEdgeFade { viewport: root.flickable }
    readonly property var toolbarOptions: [
        {name:i18n("None"),value:Private.KCM.NoText},
        {name:i18n("Text only"),value:Private.KCM.TextOnly},
        {name:i18n("Beside icons"),value:Private.KCM.TextBesideIcon},
        {name:i18n("Below icon"),value:Private.KCM.TextUnderIcon}
    ]
    function selectStyle(model) {
        if (model.isUnionStyle) { settings.widgetStyle = "Union"; settings.unionStyle = model.styleId; }
        else { settings.widgetStyle = model.styleId; settings.unionStyle = settings.defaultUnionStyleValue; }
    }
    function setUninstall(id,remove) {
        const pending = Array.from(kcm.stylesToUninstall);
        const index = pending.indexOf(id);
        if (remove && index < 0) pending.push(id);
        else if (!remove && index >= 0) pending.splice(index,1);
        kcm.stylesToUninstall = pending;
    }
    function showMessage(type,text) { message.type=type; message.text=text; message.visible=true; }
    function goBack() { if (gtkMode) { gtkMode=false; flickable.contentY=0; } else Qt.callLater(navigation.back); }
    function openGtkStyleSettings() { if (kcm.gtkConfigKdedModuleLoaded) { gtkMode=true; flickable.contentY=0; } }
    Connections {
        target: kcm
        function onShowErrorMessage(text) { root.showMessage(K.MessageType.Error,text); }
        function onShowInfoMessage(text) { root.showMessage(K.MessageType.Information,text); }
    }
    header: Item {
        implicitHeight: headerBody.implicitHeight + 24
        ColumnLayout {
            id: headerBody
            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; y: 12; spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                S.CircleButton { objectName: "applicationStyleBack"; theme: root.theme; label: i18n("Back"); icon.name: "go-previous"; onClicked: root.goBack() }
                K.Heading { Layout.fillWidth: true; color: root.theme.textColor; level: 1; font.weight: Font.DemiBold; text: root.gtkMode ? i18n("GNOME/GTK Application Style") : i18n("Application Style"); elide: Text.ElideRight }
                S.ActionPill {
                    objectName: "applicationStyleInstall"; theme: root.theme; visible: !root.gtkMode
                    text: root.width>670 ? i18n("Install From File…") : ""; icon.name: "document-import-symbolic"
                    Accessible.name: i18n("Install Application Style From File")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: installDialog.open()
                }
            }
            Flow {
                visible: !root.gtkMode; Layout.fillWidth: true; spacing: 10
                S.ActionPill {
                    objectName: "applicationStyleEffects"; theme: root.theme
                    text: i18n("Configure Icons and Toolbars…"); icon.name: "configure-toolbars"; checked: effects.visible
                    onClicked: effects.visible ? effects.close() : effects.open()
                }
                S.ActionPill {
                    objectName: "applicationStyleGtk"; theme: root.theme
                    visible: kcm.gtkConfigKdedModuleLoaded
                    text: i18n("GNOME/GTK Application Style…"); icon.name: "configure"
                    onClicked: root.openGtkStyleSettings()
                }
            }
            K.InlineMessage { id: message; objectName: "applicationStyleMessage"; Layout.fillWidth: true; showCloseButton: true; visible: false }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            S.ActionPill { theme: root.theme; text: i18n("Help"); icon.name: "help-contents"; visible: (kcm.buttons & KCM.ConfigModule.Help)!==0; onClicked: moduleActions.moduleHelp() }
            S.ActionPill { objectName: "applicationStyleDefaults"; theme: root.theme; text: i18n("Defaults"); icon.name: "edit-undo"; visible: (kcm.buttons & KCM.ConfigModule.Default)!==0; enabled: !kcm.representsDefaults; onClicked: moduleActions.moduleDefaults() }
            Item { Layout.fillWidth: true }
            S.ActionPill { objectName: "applicationStyleReset"; theme: root.theme; text: i18n("Reset"); icon.name: "edit-undo"; enabled: kcm.needsSave; onClicked: moduleActions.moduleLoad() }
            S.ActionPill { objectName: "applicationStyleApply"; theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"; enabled: kcm.needsSave; onClicked: moduleActions.saveActiveModule() }
        }
    }
    ColumnLayout {
        spacing: 14
        C.Label { Layout.fillWidth: true; visible: !root.gtkMode; text: i18n("Choose how application controls look. Previews show each style’s own widgets."); color: root.theme.mutedColor; wrapMode: Text.WordWrap }
        GridLayout {
            id: styleGrid
            objectName: "applicationStyleGrid"
            Layout.fillWidth: true; visible: !root.gtkMode
            columns: Math.max(1,Math.floor((root.width-14)/350)); columnSpacing: 14; rowSpacing: 14
            // These helpers are visual Loaders, so parent them outside the grid's cells.
            KCM.SettingStateBinding { parent: root; target: styleGrid; configObject: root.settings; settingName: "widgetStyle" }
            KCM.SettingStateBinding { parent: root; target: styleGrid; configObject: root.settings; settingName: "unionStyle" }
            Repeater {
                model: kcm.model
                delegate: S.LivePreviewCard {
                    id: card
                    required property var model
                    objectName: "applicationStyleCard-" + model.styleId
                    Layout.fillWidth: true; Layout.preferredWidth: 360; Layout.minimumWidth: 0
                    Layout.alignment: Qt.AlignTop
                    theme: root.theme; text: model.display
                    previewActive: !root.gtkMode
                    readonly property bool pendingRemoval: kcm.stylesToUninstall.includes(model.styleId)
                    caption: pendingRemoval ? i18n("Will be uninstalled when you apply") : model.isUnionStyle ? i18n("Union style · experimental") : ""
                    checked: root.settings.widgetStyle === "Union" ? model.isUnionStyle && root.settings.unionStyle === model.styleId : model.styleId === root.settings.widgetStyle
                    C.ToolTip.text: model.description; C.ToolTip.visible: hovered && !menu.visible; C.ToolTip.delay: K.Units.toolTipDelay
                    onClicked: root.selectStyle(model)
                    onDoubleClicked: { root.selectStyle(model); kcm.save(); }
                    onActionsRequested: menu.visible ? menu.close() : menu.open()
                    previewComponent: Component {
                        Loader {
                            opacity: card.pendingRemoval ? .3 : 1
                            sourceComponent: card.model.isUnionStyle ? unionPreview : widgetPreview
                            Component {
                                id: widgetPreview
                                Private.PreviewItem {
                                    id: nativePreview
                                    smooth: false; styleName: card.model.styleName
                                    Connections { target: kcm; function onStyleReconfigured() { nativePreview.reload(); } }
                                }
                            }
                            Component {
                                id: unionPreview
                                NativeStyle.UnionThumbnailItem { styleId: card.model.styleId; onSelected: root.selectStyle(card.model) }
                            }
                        }
                    }
                    S.PopupMenu {
                        id: menu
                        objectName: card.objectName + "Menu"
                        theme: root.theme; trigger: card.actionsButton; backdropItem: root
                        S.MenuItem {
                            text: i18n("About Union styles"); visible: card.model.isUnionStyle
                            onTriggered: root.showMessage(K.MessageType.Information,i18n("This style uses the Union style engine, which is still under development. Some things may look or behave incorrectly."))
                        }
                        S.MenuItem { text: i18n("Configure Style…"); enabled: card.model.configurable; onTriggered: kcm.configure(card.model.display,card.model.styleName,card) }
                        S.MenuItem {
                            text: i18n("Uninstall…"); visible: !card.pendingRemoval; enabled: card.model.canUninstall
                            C.ToolTip.visible: hovered && !enabled
                            C.ToolTip.text: card.model.isUnionStyle ? i18n("Cannot uninstall system-installed styles.") : i18n("Can only uninstall Union styles.")
                            onTriggered: root.setUninstall(card.model.styleId,true)
                        }
                        S.MenuItem { text: i18n("Do not uninstall this style"); visible: card.pendingRemoval; onTriggered: root.setUninstall(card.model.styleId,false) }
                    }
                }
            }
        }
        Loader {
            Layout.fillWidth: true; active: root.gtkMode; visible: active
            sourceComponent: Component {
                ColumnLayout {
                    spacing: 14
                    property var gtk: kcm.gtkPage
                    property var themes: gtk.gtkThemesModel
                    id: gtkPage
                    Connections { target: gtkPage.gtk; function onShowErrorMessage(text) { root.showMessage(K.MessageType.Error,text); } }
                    Flow {
                        Layout.fillWidth: true; spacing: 10
                        S.ActionPill { theme: root.theme; text: i18n("Install from File…"); icon.name: "document-import-symbolic"; onClicked: gtkInstall.open() }
                        S.ActionPill { theme: root.theme; text: i18n("Get New…"); icon.name: "get-hot-new-stuff"; enabled: gtkDownloads.enabled; visible: gtkDownloads.visible; onClicked: gtkDownloads.trigger() }
                    }
                    K.Heading { text: i18n("GTK theme"); color: root.theme.textColor; level: 3 }
                    GridLayout {
                        Layout.fillWidth: true; columns: root.width<640 ? 1 : 3; rowSpacing: 10; columnSpacing: 10
                        S.ThemeSelector {
                            objectName: "applicationStyleGtkSelector"
                            theme: root.theme; backdropItem: root
                            Layout.fillWidth: true; Layout.maximumWidth: 420
                            model: gtkPage.themes; textRole: "theme-name"
                            currentIndex: model.findThemeIndex(model.selectedTheme)
                            Accessible.name: i18n("GTK theme")
                            onActivated: { model.selectedTheme=currentText; model.setSelectedThemeDirty(); }
                        }
                        S.ActionPill {
                            objectName: "applicationStyleGtkRemove"; theme: root.theme; text: i18n("Remove"); icon.name: "edit-delete"
                            enabled: { const selected=gtkPage.themes.selectedTheme; return gtkPage.themes.selectedThemeRemovable(); }
                            onClicked: gtkPage.themes.removeSelectedTheme()
                        }
                        S.ActionPill { objectName: "applicationStyleGtkPreview"; theme: root.theme; text: i18n("Preview…"); icon.name: "view-preview"; visible: gtkPage.gtk.gtkPreviewAvailable(); onClicked: gtkPage.gtk.showGtkPreview() }
                    }
                    NewStuff.Action { id: gtkDownloads; configFile: "gtk_themes.knsrc"; onEntryEvent: (entry,event) => { if(event===NewStuff.Entry.StatusChangedEvent) kcm.load(); } }
                    Dialogs.FileDialog {
                        id: gtkInstall
                        title: i18n("Select GTK Theme Archive")
                        nameFilters: [i18n("GTK Theme Archive (*.tar.xz *.tar.gz *.tar.bz2)")]
                        onAccepted: gtkPage.gtk.installGtkThemeFromFile(selectedFile)
                    }
                }
            }
        }
    }
    S.SurfaceDialog {
        id: effects; objectName: "applicationStyleEffectsDialog"
        theme: root.theme; backdropItem: root; title: i18n("Configure Icons and Toolbars")
        modal: false; fitContentWidth: true
        onOpened: iconsOnButtons.forceActiveFocus()
        contentItem: C.ScrollView {
            id: optionsScroll; implicitWidth: options.implicitWidth; implicitHeight: options.implicitHeight; contentWidth: availableWidth; clip: true
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            ColumnLayout {
                id: options; width: optionsScroll.availableWidth; spacing: 12
                K.Heading { text: i18n("Show icons"); level: 3; color: root.theme.textColor }
                S.CheckPill {
                    id: iconsOnButtons; objectName: "applicationStyleButtonIcons"; theme: root.theme; Layout.fillWidth: true
                    text: i18n("On buttons"); checked: root.settings.iconsOnButtons
                    onToggled: root.settings.iconsOnButtons=checked
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "iconsOnButtons" }
                }
                S.CheckPill {
                    objectName: "applicationStyleMenuIcons"; theme: root.theme; Layout.fillWidth: true
                    text: i18n("In menus"); checked: root.settings.iconsInMenus
                    onToggled: root.settings.iconsInMenus=checked
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "iconsInMenus" }
                }
                C.Label { text: i18n("Main toolbar label"); color: root.theme.textColor }
                S.ThemeSelector {
                    objectName: "applicationStyleMainToolbar"; theme: root.theme; backdropItem: root; Layout.fillWidth: true
                    model: root.toolbarOptions; currentIndex: model.findIndex(item=>item.value===kcm.mainToolBarStyle)
                    Accessible.name: i18n("Main toolbar label")
                    onActivated: kcm.mainToolBarStyle=model[currentIndex].value
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "toolButtonStyle" }
                }
                C.Label { text: i18n("Secondary toolbar label"); color: root.theme.textColor }
                S.ThemeSelector {
                    objectName: "applicationStyleSecondaryToolbar"; theme: root.theme; backdropItem: root; Layout.fillWidth: true
                    model: root.toolbarOptions; currentIndex: model.findIndex(item=>item.value===kcm.otherToolBarStyle)
                    Accessible.name: i18n("Secondary toolbar label")
                    onActivated: kcm.otherToolBarStyle=model[currentIndex].value
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "toolButtonStyleOtherToolbars" }
                }
            }
        }
        footer: Item {
            implicitHeight: 66
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Item { Layout.fillWidth: true }
                S.ActionPill { objectName: "applicationStyleEffectsDone"; theme: root.theme; text: i18n("Done"); icon.name: "checkmark-symbolic"; onClicked: effects.close() }
            }
        }
    }
    Dialogs.FileDialog {
        id: installDialog; title: i18n("Choose Union Style to Install"); acceptLabel: i18n("Install")
        options: Dialogs.FileDialog.ReadOnly; nameFilters: ["Union Styles (*.unionstyle)"]
        onAccepted: kcm.installUnionStyle(selectedFile)
    }
}
