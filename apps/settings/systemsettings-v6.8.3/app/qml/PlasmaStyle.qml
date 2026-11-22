// SPDX-License-Identifier: GPL-2.0-or-later
// Presentation only: KDE's Plasma Style KCM owns the model and configuration.
import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls as C
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as K
import org.kde.kcmutils as KCM
import org.kde.newstuff as NewStuff
import org.kde.private.kcms.desktoptheme as Private
import org.sequoia.components 1.0 as S
import "qrc:/kcm/kcm_desktoptheme" as NativeTheme
import "qrc:/qt/qml/org/kde/systemsettings" as Hub

KCM.SimpleKCM {
    id: root
    objectName: "plasmaStyleHub"
    title: ""; globalToolBarStyle: K.ApplicationHeaderStyle.None
    verticalScrollBarPolicy: C.ScrollBar.AlwaysOff
    implicitWidth: 820; implicitHeight: 680
    leftPadding: 14; rightPadding: 14; topPadding: 12; bottomPadding: 12
    property S.Theme theme: Hub.AppearanceLibraryTheme {}
    property var backend: kcm
    property var settings: kcm.desktopThemeSettings
    property Hub.ScrollEdgeFade edgeFade: Hub.ScrollEdgeFade { viewport: root.flickable }
    readonly property var filters: [
        {name:i18n("All themes"),value:Private.FilterProxyModel.AllThemes},
        {name:i18n("Light themes"),value:Private.FilterProxyModel.LightThemes},
        {name:i18n("Dark themes"),value:Private.FilterProxyModel.DarkThemes},
        {name:i18n("Color scheme compatible"),value:Private.FilterProxyModel.ThemesFollowingColors}
    ]
    Binding { target: kcm.filteredModel; property: "query"; value: search.text; restoreMode: Binding.RestoreBinding }
    Binding { target: kcm.filteredModel; property: "filter"; value: root.filters[filter.currentIndex].value; restoreMode: Binding.RestoreBinding }
    function showMessage(type,text) { message.severity=type; message.text=text; message.visible=true; }
    Connections {
        target: kcm
        function onShowSuccessMessage(text) { root.showMessage("success",text); }
        function onShowErrorMessage(text) { root.showMessage("error",text); }
    }
    // Keep this visual Loader outside the grid, so it cannot reserve a card slot.
    KCM.SettingStateBinding { parent: root; target: themeGrid; configObject: root.settings; settingName: "name"; extraEnabledConditions: !kcm.downloadingFile }
    DropArea {
        parent: root; anchors.fill: parent
        enabled: !kcm.downloadingFile
        onEntered: drag => { if (!drag.hasUrls) drag.accepted=false; }
        onDropped: drop => { if (drop.hasUrls && drop.urls.length) kcm.installThemeFromFile(drop.urls[0]); }
    }
    NewStuff.Action { id: downloads; transientParent: root.Window.window; configFile: "plasma-themes.knsrc"; viewMode: NewStuff.Page.Preview; onEntryEvent: (entry,event) => kcm.load() }
    header: Item {
        implicitHeight: headerBody.implicitHeight + 24
        ColumnLayout {
            id: headerBody
            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; y: 12; spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                S.CircleButton { objectName: "plasmaStyleBack"; theme: root.theme; label: i18n("Back"); icon.name: "go-previous"; onClicked: Qt.callLater(navigation.back) }
                K.Heading { Layout.fillWidth: true; color: root.theme.textColor; text: i18n("Plasma Style"); level: 1; font.weight: Font.DemiBold; elide: Text.ElideRight }
                S.ActionPill {
                    objectName: "plasmaStyleInstall"; theme: root.theme; icon.name: "document-import-symbolic"
                    text: root.width>700 ? i18n("Install from File…") : ""; enabled: !kcm.downloadingFile
                    Accessible.name: i18n("Install Plasma Style from File")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: installDialog.open()
                }
                S.ActionPill {
                    objectName: "plasmaStyleGetNew"; theme: root.theme; icon.name: "get-hot-new-stuff"
                    text: root.width>700 ? i18n("Get New…") : ""; enabled: downloads.enabled && !kcm.downloadingFile; visible: downloads.visible
                    Accessible.name: i18n("Get New Plasma Styles")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: downloads.trigger()
                }
            }
            GridLayout {
                Layout.fillWidth: true; Layout.maximumWidth: 712; Layout.alignment: Qt.AlignLeft
                columns: root.width<660 ? 1 : 2; columnSpacing: 12; rowSpacing: 10
                S.SearchField {
                    id: search; objectName: "plasmaStyleSearch"; theme: root.theme
                    Layout.fillWidth: true; Layout.maximumWidth: 420; Layout.minimumWidth: 0
                    placeholderText: i18n("Search Plasma styles…"); Accessible.name: i18n("Search Plasma styles")
                    onTextChanged: root.flickable.contentY=0
                }
                S.ThemeSelector {
                    id: filter; objectName: "plasmaStyleFilter"; theme: root.theme; backdropItem: root
                    Layout.fillWidth: true; Layout.maximumWidth: 280; Layout.minimumWidth: 0
                    model: root.filters; currentIndex: 0; Accessible.name: i18n("Filter Plasma styles")
                    onActivated: root.flickable.contentY=0
                }
            }
            C.Label { visible: kcm.downloadingFile; text: i18n("Installing Plasma style…"); color: root.theme.mutedColor }
            S.MessagePanel { id: message; objectName: "plasmaStyleMessage"; theme: root.theme; Layout.fillWidth: true; Layout.maximumWidth: 712; visible: false; onDismissed: visible=false }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            S.ActionPill { theme: root.theme; text: i18n("Help"); icon.name: "help-contents"; visible: (kcm.buttons & KCM.ConfigModule.Help)!==0; onClicked: moduleActions.moduleHelp() }
            S.ActionPill { objectName: "plasmaStyleDefaults"; theme: root.theme; text: i18n("Defaults"); icon.name: "edit-undo"; enabled: !kcm.representsDefaults && !kcm.downloadingFile; visible: (kcm.buttons & KCM.ConfigModule.Default)!==0; onClicked: moduleActions.moduleDefaults() }
            Item { Layout.fillWidth: true }
            S.ActionPill { objectName: "plasmaStyleReset"; theme: root.theme; text: i18n("Reset"); icon.name: "edit-undo"; enabled: kcm.needsSave && !kcm.downloadingFile; onClicked: moduleActions.moduleLoad() }
            S.ActionPill { objectName: "plasmaStyleApply"; theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"; enabled: kcm.needsSave && !kcm.downloadingFile; onClicked: moduleActions.saveActiveModule() }
        }
    }
    ColumnLayout {
        spacing: 14
        C.Label { Layout.fillWidth: true; color: root.theme.mutedColor; wrapMode: Text.WordWrap; text: i18n("Choose the appearance of your panels and desktop widgets.") }
        GridLayout {
            id: themeGrid; objectName: "plasmaStyleGrid"
            Layout.fillWidth: true; Layout.maximumWidth: columns * 360 + (columns-1) * columnSpacing; Layout.alignment: Qt.AlignLeft
            columns: Math.max(1,Math.floor((root.width-14)/320)); columnSpacing: 14; rowSpacing: 14
            Repeater {
                id: styles; model: kcm.filteredModel
                delegate: S.LivePreviewCard {
                    id: card
                    required property var model
                    objectName: "plasmaStyleCard-" + model.pluginName
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 320; Layout.maximumWidth: 360; Layout.alignment: Qt.AlignTop
                    theme: root.theme; text: model.display; previewHeight: 180
                    readonly property bool pendingRemoval: model.pendingDeletion
                    checked: root.settings.name===model.pluginName
                    caption: pendingRemoval ? i18n("Will be removed when you apply") : model.colorType===Private.ThemesModel.FollowsColorTheme && filter.currentIndex!==3 ? i18n("Follows color scheme") : ""
                    C.ToolTip.text: model.description || model.display; C.ToolTip.visible: hovered && !actions.visible; C.ToolTip.delay: K.Units.toolTipDelay
                    onClicked: root.settings.name=model.pluginName
                    onDoubleClicked: { root.settings.name=model.pluginName; kcm.save(); }
                    onActionsRequested: actions.visible ? actions.close() : actions.open()
                    previewComponent: Component {
                        NativeTheme.ThemePreview {
                            property var model: card.model
                            themeName: card.model.pluginName
                            opacity: card.pendingRemoval ? .3 : 1
                        }
                    }
                    S.PopupMenu {
                        id: actions; objectName: card.objectName + "Menu"
                        theme: root.theme; trigger: card.actionsButton; backdropItem: root
                        S.MenuItem { text: i18n("Edit Theme…"); visible: kcm.canEditThemes; enabled: !card.pendingRemoval; onTriggered: kcm.editTheme(card.model.pluginName) }
                        S.MenuItem {
                            text: i18n("Remove Plasma style"); visible: !card.pendingRemoval; enabled: card.model.isLocal && !card.checked
                            C.ToolTip.visible: hovered && !enabled
                            C.ToolTip.text: card.checked ? i18n("Cannot delete the active Plasma style") : i18n("Cannot delete system-installed Plasma styles")
                            onTriggered: card.model.pendingDeletion=true
                        }
                        S.MenuItem { text: i18n("Don’t delete this Plasma style"); visible: card.pendingRemoval; onTriggered: card.model.pendingDeletion=false }
                    }
                }
            }
        }
        C.Label { objectName: "plasmaStyleEmpty"; visible: styles.count===0; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.theme.mutedColor; text: i18n("No Plasma styles match your search or filter.") }
        S.ActionPill { objectName: "plasmaStyleClearFilters"; visible: styles.count===0 && (search.text.length>0 || filter.currentIndex!==0); theme: root.theme; text: i18n("Clear filters"); icon.name: "edit-clear"; onClicked: { search.clear(); filter.currentIndex=0; } }
    }
    Dialogs.FileDialog {
        id: installDialog; objectName: "plasmaStyleInstallDialog"
        title: i18n("Open Theme"); currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        nameFilters: [i18n("Theme Files (*.zip *.tar.gz *.tar.bz2)")]
        onAccepted: kcm.installThemeFromFile(selectedFile)
    }
}
