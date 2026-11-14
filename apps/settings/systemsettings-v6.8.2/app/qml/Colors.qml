// SPDX-License-Identifier: GPL-2.0-or-later
// Presentation only: KDE's Colors KCM owns the model and configuration.
import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls as C
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as K
import org.kde.kcmutils as KCM
import org.kde.newstuff as NewStuff
import org.kde.private.kcms.colors as Private
import org.sequoia.components 1.0 as S
import "qrc:/qt/qml/org/kde/systemsettings" as Hub

KCM.SimpleKCM {
    id: root
    objectName: "colorsHub"
    title: ""; globalToolBarStyle: K.ApplicationHeaderStyle.None
    verticalScrollBarPolicy: C.ScrollBar.AlwaysOff
    implicitWidth: 820; implicitHeight: 680
    leftPadding: 14; rightPadding: 14; topPadding: 12; bottomPadding: 12
    property S.Theme theme: Hub.AppearanceLibraryTheme {}
    property var backend: kcm
    property var settings: kcm.colorsSettings
    property Hub.ScrollEdgeFade edgeFade: Hub.ScrollEdgeFade { viewport: root.flickable }
    readonly property var filters: [
        {name:i18n("All schemes"),value:Private.KCM.AllSchemes},
        {name:i18n("Light schemes"),value:Private.KCM.LightSchemes},
        {name:i18n("Dark schemes"),value:Private.KCM.DarkSchemes}
    ]
    readonly property var accent: Qt.colorEqual(kcm.accentColor,"transparent") ? undefined : kcm.accentColor
    function chooseAccent(color) {
        kcm.accentColorFromWallpaper=false;
        kcm.accentColor=color;
        kcm.lastUsedCustomAccentColor=color;
    }
    function chooseAccentMode(index) {
        if (index===0) { kcm.accentColorFromWallpaper=false; kcm.accentColor="transparent"; }
        else if (index===1) kcm.accentColorFromWallpaper=true;
        else chooseAccent(Qt.colorEqual(kcm.lastUsedCustomAccentColor,"transparent") ? "#3daee9" : kcm.lastUsedCustomAccentColor);
    }
    function previewColors(model) {
        const p=model.palette, tint=c=>kcm.tinted(c,kcm.accentColor,model.tints,model.tintFactor);
        const highlight=accent ? kcm.accentBackground(accent,p.base) : p.highlight;
        return {window:tint(p.window),windowText:p.windowText,base:tint(p.base),text:tint(p.text),
            button:tint(p.button),buttonText:tint(p.buttonText),disabledText:tint(model.disabledText),
            highlight:tint(highlight),highlightedText:tint(accent ? kcm.accentForeground(highlight,true) : p.highlightedText),
            link:tint(accent || p.link),visited:p.linkVisited,highlightLink:model.selectedPalette.link,highlightVisited:model.selectedPalette.linkVisited,
            titleBackground:tint(model.accentActiveTitlebar && accent ? kcm.accentBackground(accent,p.window) : model.activeTitleBarBackground),
            titleForeground:model.accentActiveTitlebar && accent ? kcm.accentForeground(kcm.accentBackground(accent,p.window),true) : model.activeTitleBarForeground};
    }
    Binding { target: kcm.filteredModel; property: "query"; value: search.text; restoreMode: Binding.RestoreBinding }
    Binding { target: kcm.filteredModel; property: "filter"; value: root.filters[filter.currentIndex].value; restoreMode: Binding.RestoreBinding }
    function showMessage(type,text) { message.severity=type; message.text=text; message.visible=true; }
    Connections {
        target: kcm
        function onShowSchemeNotInstalledWarning(name) { root.showMessage("warning",i18n("The color scheme %1 is not installed. Selecting the default scheme instead.",name)); }
        function onShowSuccessMessage(text) { root.showMessage("success",text); }
        function onShowErrorMessage(text) { root.showMessage("error",text); }
    }
    // Keep this visual Loader outside the grid, so it cannot reserve a card slot.
    KCM.SettingStateBinding { parent: root; target: themeGrid; configObject: root.settings; settingName: "colorScheme"; extraEnabledConditions: !kcm.downloadingFile }
    DropArea {
        parent: root; anchors.fill: parent
        enabled: !kcm.downloadingFile
        onEntered: drag => { if (!drag.hasUrls) drag.accepted=false; }
        onDropped: drop => { if (drop.hasUrls && drop.urls.length) kcm.installSchemeFromFile(drop.urls[0]); }
    }
    NewStuff.Action { id: downloads; configFile: "colorschemes.knsrc"; viewMode: NewStuff.Page.Preview; onEntryEvent: (entry,event) => { if (event===NewStuff.Entry.StatusChangedEvent) kcm.knsEntryChanged(entry); else if (event===NewStuff.Entry.AdoptedEvent) kcm.loadSelectedColorScheme(); } }
    header: Item {
        implicitHeight: headerBody.implicitHeight + 24
        ColumnLayout {
            id: headerBody
            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; y: 12; spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                S.CircleButton { objectName: "colorsBack"; theme: root.theme; label: i18n("Back"); icon.name: "go-previous"; onClicked: Qt.callLater(navigation.back) }
                K.Heading { Layout.fillWidth: true; color: root.theme.textColor; text: i18n("Colors"); level: 1; font.weight: Font.DemiBold; elide: Text.ElideRight }
                S.ActionPill {
                    objectName: "colorsInstall"; theme: root.theme; icon.name: "document-import-symbolic"
                    text: root.width>700 ? i18n("Install from File…") : ""; enabled: !kcm.downloadingFile
                    Accessible.name: i18n("Install Color Scheme from File")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: installDialog.open()
                }
                S.ActionPill {
                    objectName: "colorsGetNew"; theme: root.theme; icon.name: "get-hot-new-stuff"
                    text: root.width>700 ? i18n("Get New…") : ""; enabled: downloads.enabled && !kcm.downloadingFile; visible: downloads.visible
                    Accessible.name: i18n("Get New Color Schemes")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: downloads.trigger()
                }
            }
            GridLayout {
                Layout.fillWidth: true; Layout.maximumWidth: 712; Layout.alignment: Qt.AlignLeft
                columns: root.width<660 ? 1 : 2; columnSpacing: 12; rowSpacing: 10
                S.SearchField {
                    id: search; objectName: "colorsSearch"; theme: root.theme
                    Layout.fillWidth: true; Layout.maximumWidth: 420; Layout.minimumWidth: 0
                    placeholderText: i18n("Search color schemes…"); Accessible.name: i18n("Search color schemes")
                    onTextChanged: root.flickable.contentY=0
                }
                S.ThemeSelector {
                    id: filter; objectName: "colorsFilter"; theme: root.theme; backdropItem: root
                    Layout.fillWidth: true; Layout.maximumWidth: 280; Layout.minimumWidth: 0
                    model: root.filters; currentIndex: 0; Accessible.name: i18n("Filter color schemes")
                    onActivated: root.flickable.contentY=0
                }
            }
            C.Label { visible: kcm.downloadingFile; text: i18n("Installing color scheme…"); color: root.theme.mutedColor }
            S.MessagePanel { id: message; objectName: "colorsMessage"; theme: root.theme; Layout.fillWidth: true; Layout.maximumWidth: 712; visible: false; onDismissed: visible=false }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            S.ActionPill { theme: root.theme; text: i18n("Help"); icon.name: "help-contents"; visible: (kcm.buttons & KCM.ConfigModule.Help)!==0; onClicked: moduleActions.moduleHelp() }
            S.ActionPill { objectName: "colorsDefaults"; theme: root.theme; text: i18n("Defaults"); icon.name: "edit-undo"; enabled: !kcm.representsDefaults && !kcm.downloadingFile; visible: (kcm.buttons & KCM.ConfigModule.Default)!==0; onClicked: moduleActions.moduleDefaults() }
            Item { Layout.fillWidth: true }
            S.ActionPill { objectName: "colorsReset"; theme: root.theme; text: i18n("Reset"); icon.name: "edit-undo"; enabled: kcm.needsSave && !kcm.downloadingFile; onClicked: moduleActions.moduleLoad() }
            S.ActionPill { objectName: "colorsApply"; theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"; enabled: kcm.needsSave && !kcm.downloadingFile; onClicked: moduleActions.saveActiveModule() }
        }
    }
    ColumnLayout {
        spacing: 14
        C.Label { Layout.fillWidth: true; color: root.theme.mutedColor; wrapMode: Text.WordWrap; text: i18n("Choose your application colors. Previews show each scheme with your selected accent.") }
        C.Label { text: i18n("Accent color"); color: root.theme.textColor; font.bold: true }
        S.Panel {
            Layout.fillWidth: true; Layout.maximumWidth: Math.min(860,10*(S.Tokens.controlHeight+6)+customAccentButton.implicitWidth+24); Layout.alignment: Qt.AlignLeft
            implicitHeight: accentControls.implicitHeight+24; radius: 24
            theme: root.theme
            ColumnLayout {
                id: accentControls; anchors.fill: parent; anchors.margins: 12; spacing: 10
                S.ThemeSelector {
                    objectName: "colorsAccentMode"; theme: root.theme; backdropItem: root
                    Layout.fillWidth: true; Layout.maximumWidth: 300
                    model: [{name:i18n("Accent from color scheme")},{name:i18n("Accent from wallpaper")},{name:i18n("Custom accent color")}]
                    currentIndex: kcm.accentColorFromWallpaper ? 1 : Qt.colorEqual(kcm.accentColor,"transparent") ? 0 : 2
                    Accessible.name: i18n("Accent color source")
                    onActivated: index=>root.chooseAccentMode(index)
                }
                Flow {
                    Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 6
                    Repeater {
                        model: ["#e93a9a","#e93d58","#e9643a","#e8cb2d","#3dd425","#00d3b8","#3daee9","#b875dc","#926ee4","#686b6f"]
                        S.ColorSwatch {
                            required property string modelData
                            objectName: "colorsAccent-"+modelData.slice(1)
                            theme: root.theme; color: modelData
                            checked: !kcm.accentColorFromWallpaper && Qt.colorEqual(kcm.accentColor,modelData)
                            onClicked: root.chooseAccent(modelData)
                        }
                    }
                    S.ActionPill {
                        id: customAccentButton; objectName: "colorsCustomAccent"; theme: root.theme; text: i18n("Custom color…"); icon.name: "color-picker"
                        onClicked: { colorDialog.chooseColor(root.accent || (Qt.colorEqual(kcm.lastUsedCustomAccentColor,"transparent") ? "#3daee9" : kcm.lastUsedCustomAccentColor)); colorDialog.open(); }
                    }
                }
            }
        }
        C.Label { text: i18n("Color schemes"); color: root.theme.textColor; font.bold: true }
        GridLayout {
            id: themeGrid; objectName: "colorsGrid"
            Layout.fillWidth: true; Layout.maximumWidth: columns * 360 + (columns-1) * columnSpacing; Layout.alignment: Qt.AlignLeft
            columns: Math.max(1,Math.floor((root.width-14)/320)); columnSpacing: 14; rowSpacing: 14
            Repeater {
                id: styles; model: kcm.filteredModel
                delegate: S.LivePreviewCard {
                    id: card
                    required property var model
                    objectName: "colorsCard-" + model.schemeName
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 320; Layout.maximumWidth: 360; Layout.alignment: Qt.AlignTop
                    theme: root.theme; text: model.display; previewHeight: 210
                    readonly property bool pendingRemoval: model.pendingDeletion
                    checked: kcm.model.selectedScheme===model.schemeName
                    caption: pendingRemoval ? i18n("Will be removed when you apply") : ""
                    C.ToolTip.text: model.display; C.ToolTip.visible: hovered && !actions.visible; C.ToolTip.delay: K.Units.toolTipDelay
                    onClicked: kcm.model.selectedScheme=model.schemeName
                    onDoubleClicked: { kcm.model.selectedScheme=model.schemeName; kcm.save(); }
                    onActionsRequested: actions.visible ? actions.close() : actions.open()
                    previewComponent: Component {
                        S.ColorSchemePreview { theme: root.theme; colors: root.previewColors(card.model); opacity: card.pendingRemoval ? .3 : 1 }
                    }
                    S.PopupMenu {
                        id: actions; objectName: card.objectName + "Menu"
                        theme: root.theme; trigger: card.actionsButton; backdropItem: root
                        S.MenuItem { text: i18n("Edit Color Scheme…"); enabled: !card.pendingRemoval; onTriggered: kcm.editScheme(card.model.schemeName,root) }
                        S.MenuItem {
                            text: i18n("Remove color scheme"); visible: !card.pendingRemoval; enabled: card.model.removable && !card.checked
                            C.ToolTip.visible: hovered && !enabled
                            C.ToolTip.text: card.checked ? i18n("Cannot delete the active color scheme") : i18n("Cannot delete system-installed color schemes")
                            onTriggered: card.model.pendingDeletion=true
                        }
                        S.MenuItem { text: i18n("Don’t delete this color scheme"); visible: card.pendingRemoval; onTriggered: card.model.pendingDeletion=false }
                    }
                }
            }
        }
        C.Label { objectName: "colorsEmpty"; visible: styles.count===0; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.theme.mutedColor; text: i18n("No color schemes match your search or filter.") }
        S.ActionPill { objectName: "colorsClearFilters"; visible: styles.count===0 && (search.text.length>0 || filter.currentIndex!==0); theme: root.theme; text: i18n("Clear filters"); icon.name: "edit-clear"; onClicked: { search.clear(); filter.currentIndex=0; } }
    }
    S.ColorDialog {
        id: colorDialog; objectName: "colorsColorDialog"; namePrefix: "colorsCustomAccent"
        theme: root.theme; backdropItem: root; title: i18n("Choose accent color")
        onAccepted: root.chooseAccent(selectedColor)
    }
    Dialogs.FileDialog {
        id: installDialog; objectName: "colorsInstallDialog"
        title: i18n("Open Color Scheme"); currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        nameFilters: [i18n("Color Scheme Files (*.colors)")]
        onAccepted: kcm.installSchemeFromFile(selectedFile)
    }
}
