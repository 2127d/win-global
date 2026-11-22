// SPDX-License-Identifier: GPL-2.0-or-later
// Sequoia presentation for KDE's installed Global Theme KCM.
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs as Dialogs
import org.kde.kirigami as K
import org.kde.kcmutils as KCM
import org.kde.config as KConfig
import org.kde.kitemmodels as Models
import org.kde.newstuff as NewStuff
import org.kde.private.kcms.lookandfeel as Private
import org.sequoia.components 1.0 as S
import "qrc:/qt/qml/org/kde/systemsettings" as Hub

KCM.SimpleKCM {
    id: root
    objectName: "globalThemeHub"
    title: ""
    globalToolBarStyle: K.ApplicationHeaderStyle.None
    verticalScrollBarPolicy: C.ScrollBar.AlwaysOff
    implicitWidth: 820; implicitHeight: 680
    leftPadding: 14; rightPadding: 14; topPadding: 12; bottomPadding: 12
    property S.Theme theme: Hub.AppearanceLibraryTheme {}
    property var backend: kcm
    property var settings: kcm.settings
    property string subpage: ""
    property string removalPackage: ""
    property url snapshotPreview
    readonly property bool selecting: subpage === "light" || subpage === "dark"
    readonly property bool automatic: settings.automaticLookAndFeel
    readonly property int selectionVariant: subpage === "light" ? Private.LookAndFeel.Variant.Light : Private.LookAndFeel.Variant.Dark
    readonly property bool hasAppearance: (kcm.themeContents & Private.LookandFeelManager.AppearanceSettings) !== 0
    readonly property bool hasLayout: (kcm.themeContents & Private.LookandFeelManager.LayoutSettings) !== 0
    property Hub.ScrollEdgeFade edgeFade: Hub.ScrollEdgeFade { viewport: root.flickable }
    KCM.ConfigModule.buttons: KCM.ConfigModule.Default | KCM.ConfigModule.Help | (automatic ? KCM.ConfigModule.Apply : 0)

    function goBack() {
        if (subpage.length) { subpage = ""; flickable.contentY = 0; }
        else Qt.callLater(navigation.back);
    }
    function showConfirmation() {
        subpage = "";
        applySheet.detailed = false;
        kcm.selectedItems = undefined;
        applySheet.open();
        confirmApply.forceActiveFocus();
    }
    function toggleContents(flag) { kcm.selectedContents ^= flag; }
    function showThemePreview(path) { previewImage.source = path; themePreview.showFullScreen(); }
    function choosePackage(packageId) {
        if (selecting) {
            if (subpage === "light") settings.defaultLightLookAndFeel = packageId;
            else settings.defaultDarkLookAndFeel = packageId;
            subpage = ""; flickable.contentY = 0;
        } else {
            settings.lookAndFeelPackage = packageId;
            showConfirmation();
        }
    }
    Connections { target: kcm; function onShowConfirmation() { root.showConfirmation(); } }
    Private.LookAndFeelInformation { id: selectedInfo; model: kcm.model; packageId: root.settings.lookAndFeelPackage }
    Private.LookAndFeelInformation { id: lightInfo; model: kcm.model; packageId: root.settings.defaultLightLookAndFeel }
    Private.LookAndFeelInformation { id: darkInfo; model: kcm.model; packageId: root.settings.defaultDarkLookAndFeel }
    Models.KSortFilterProxyModel {
        id: variantModel
        sourceModel: kcm.model; filterRoleName: "variant"
        filterRowCallback: {
            // Capture the variant in the binding so changing selector pages
            // invalidates the proxy's filter, not only the callback's result.
            const variant = root.selectionVariant;
            return (sourceRow, sourceParent) => {
                const value = sourceModel.data(sourceModel.index(sourceRow,0,sourceParent),filterRole);
                return value === Private.LookAndFeel.Variant.Unknown || value === variant;
            };
        }
    }
    NewStuff.Action {
        id: getThemes
        configFile: "lookandfeel.knsrc"
        onEntryEvent: (entry,event) => {
            if (event === NewStuff.Entry.StatusChangedEvent) kcm.knsEntryChanged(entry);
            else if (event === NewStuff.Entry.AdoptedEvent) kcm.reloadConfig();
        }
    }
    header: Item {
        implicitHeight: headerContent.implicitHeight + 24
        ColumnLayout {
            id: headerContent
            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 14; y: 12; spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                S.CircleButton { objectName: "globalThemeBack"; theme: root.theme; label: i18n("Back"); icon.name: "go-previous"; onClicked: root.goBack() }
                K.Heading {
                    Layout.fillWidth: true; level: 1; font.weight: Font.DemiBold; color: root.theme.textColor
                    text: root.subpage === "save" ? i18n("Save Current Theme") : root.selecting ? (root.subpage === "light" ? i18n("Choose Light Global Theme") : i18n("Choose Dark Global Theme")) : i18n("Global Theme")
                    elide: Text.ElideRight
                }
                S.ActionPill {
                    objectName: "globalThemeSaveCurrent"; theme: root.theme; visible: !root.subpage.length
                    text: root.width > 760 ? i18n("Save Current Theme…") : ""; icon.name: "document-save"
                    Accessible.name: i18n("Save Current Theme")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: { root.snapshotPreview = ""; nameField.clear(); root.subpage = "save"; root.flickable.contentY = 0; }
                }
                S.ActionPill {
                    objectName: "globalThemeGetNew"; theme: root.theme; visible: !root.subpage.length && getThemes.visible
                    enabled: getThemes.enabled; text: root.width > 760 ? i18n("Get New…") : ""; icon.name: "get-hot-new-stuff"
                    Accessible.name: i18n("Get New Global Themes")
                    C.ToolTip.visible: hovered && !text.length; C.ToolTip.text: Accessible.name
                    onClicked: getThemes.trigger()
                }
            }
            S.CheckPill {
                objectName: "globalThemeAutomatic"; theme: root.theme; visible: !root.subpage.length
                text: i18n("Switch to Dark Mode at Night"); checked: root.automatic
                onToggled: {
                    root.settings.automaticLookAndFeel = checked;
                    // KDE immediately saves disabling automatic switching; enabling
                    // it stages the pair of packages and idle options for Apply.
                    if (!checked) kcm.save();
                }
            }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            S.ActionPill { theme: root.theme; text: i18n("Help"); icon.name: "help-contents"; onClicked: moduleActions.moduleHelp() }
            S.ActionPill { objectName: "globalThemeDefaults"; theme: root.theme; text: i18n("Defaults"); icon.name: "edit-undo"; enabled: !kcm.representsDefaults; onClicked: moduleActions.moduleDefaults() }
            Item { Layout.fillWidth: true }
            S.ActionPill { objectName: "globalThemeReset"; theme: root.theme; text: i18n("Reset"); icon.name: "edit-undo"; visible: root.automatic; enabled: kcm.needsSave; onClicked: moduleActions.moduleLoad() }
            S.ActionPill { objectName: "globalThemeApply"; theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"; visible: root.automatic; enabled: kcm.needsSave; onClicked: moduleActions.saveActiveModule() }
        }
    }
    ColumnLayout {
        spacing: 14
        C.Label {
            Layout.fillWidth: true; color: root.theme.mutedColor; wrapMode: Text.WordWrap
            visible: !root.subpage.length && !root.automatic
            text: i18n("Choose a theme, then choose which parts to apply.")
        }
        GridLayout {
            id: themeGrid
            objectName: "globalThemeGrid"
            Layout.fillWidth: true; columns: Math.max(1,Math.floor((root.width - 14)/234))
            rowSpacing: 14; columnSpacing: 14
            visible: root.selecting || (!root.automatic && !root.subpage.length)
            Repeater {
                model: root.selecting ? variantModel : kcm.model
                delegate: S.ThemePreviewCard {
                    id: card
                    required property var model
                    objectName: "globalThemeCard-" + model.pluginName
                    Layout.fillWidth: true; Layout.preferredWidth: 250; Layout.minimumWidth: 0
                    theme: root.theme; text: model.display; wrapTitle: true
                    caption: model.hasDesktopLayout ? i18n("Contains Desktop layout") : ""
                    previewSource: model.screenshot || ""
                    checkable: false; autoExclusive: false
                    checked: model.pluginName === (root.selecting ? (root.subpage === "light" ? root.settings.defaultLightLookAndFeel : root.settings.defaultDarkLookAndFeel) : root.settings.lookAndFeelPackage)
                    onClicked: root.choosePackage(model.pluginName)
                    onChangeRequested: actionsMenu.visible ? actionsMenu.close() : actionsMenu.open()
                    Binding { target: card.changeButton; property: "ellipsis"; value: true }
                    Binding { target: card.changeButton; property: "label"; value: i18n("Theme actions") }
                    C.ToolTip.text: model.description; C.ToolTip.visible: hovered && !actionsMenu.visible; C.ToolTip.delay: K.Units.toolTipDelay
                    S.PopupMenu {
                        id: actionsMenu
                        theme: root.theme; trigger: card.changeButton; backdropItem: root
                        S.MenuItem { text: i18n("Preview Theme"); visible: card.model.fullScreenPreview !== ""; onTriggered: root.showThemePreview(card.model.fullScreenPreview) }
                        S.MenuItem {
                            text: i18n("Remove theme"); enabled: card.model.uninstallable && !card.checked
                            C.ToolTip.visible: hovered && !enabled
                            C.ToolTip.text: card.checked ? i18n("Cannot delete the active theme") : i18n("Cannot delete system-installed themes")
                            onTriggered: { root.removalPackage = card.model.pluginName; deleteSheet.open(); }
                        }
                    }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 14
            visible: root.automatic && !root.subpage.length
            C.Label { Layout.fillWidth: true; color: root.theme.mutedColor; wrapMode: Text.WordWrap; text: i18n("The chosen light Global Theme will be used during the day, and the dark one will be used at night.") }
            S.ActionPill {
                objectName: "globalThemeCycle"; theme: root.theme; text: i18n("Configure Day/Night Cycle…"); icon.name: "configure"
                enabled: KConfig.KAuthorized.authorizeControlModule("kcm_nighttime")
                onClicked: KCM.KCMLauncher.open("kcm_nighttime")
            }
            GridLayout {
                Layout.fillWidth: true; columns: root.width < 540 ? 1 : 2; columnSpacing: 14; rowSpacing: 14
                S.ThemePreviewCard {
                    objectName: "globalThemeLight"; theme: root.theme; Layout.fillWidth: true; Layout.preferredWidth: 250
                    text: i18n("Light"); caption: lightInfo.name; previewSource: lightInfo.preview
                    checkable: false; autoExclusive: false; changeEnabled: false
                    onClicked: { root.subpage = "light"; root.flickable.contentY = 0; }
                }
                S.ThemePreviewCard {
                    objectName: "globalThemeDark"; theme: root.theme; Layout.fillWidth: true; Layout.preferredWidth: 250
                    text: i18n("Dark"); caption: darkInfo.name; previewSource: darkInfo.preview
                    checkable: false; autoExclusive: false; changeEnabled: false
                    onClicked: { root.subpage = "dark"; root.flickable.contentY = 0; }
                }
            }
            GridLayout {
                Layout.fillWidth: true; columns: root.width < 640 ? 1 : 2; rowSpacing: 12; columnSpacing: 12
                S.CheckPill {
                    objectName: "globalThemeWaitIdle"; theme: root.theme; text: i18n("Wait until idleness before switching")
                    Layout.fillWidth: true; checked: root.settings.automaticLookAndFeelOnIdle
                    onToggled: root.settings.automaticLookAndFeelOnIdle = checked
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "automaticLookAndFeelOnIdle" }
                }
                S.SpinPill {
                    objectName: "globalThemeIdleInterval"; theme: root.theme
                    from: 1; value: root.settings.automaticLookAndFeelIdleInterval
                    textFromValue: (value,locale) => i18np("%1 second", "%1 seconds",value)
                    valueFromText: (text,locale) => parseInt(text)
                    onValueModified: root.settings.automaticLookAndFeelIdleInterval = value
                    Accessible.name: i18n("Idle interval")
                    KCM.SettingStateBinding { configObject: root.settings; settingName: "automaticLookAndFeelIdleInterval"; extraEnabledConditions: root.settings.automaticLookAndFeelOnIdle }
                }
            }
            C.Label { Layout.fillWidth: true; text: i18n("Minimizes interruptions when actively using the computer."); color: root.theme.mutedColor; wrapMode: Text.WordWrap }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 14; visible: root.subpage === "save"
            C.Label { Layout.fillWidth: true; color: root.theme.mutedColor; wrapMode: Text.WordWrap; text: i18n("Plasma’s current appearance and theme settings will be saved in a new global theme.") }
            S.ThemePreviewCard {
                objectName: "globalThemeSnapshotImage"; theme: root.theme; Layout.fillWidth: true; Layout.maximumWidth: 420
                text: i18n("Add a preview image…"); previewSource: root.snapshotPreview
                checkable: false; autoExclusive: false; changeEnabled: false; onClicked: { if (String(root.snapshotPreview).length) imageChooser.selectedFile = root.snapshotPreview; imageChooser.open(); }
            }
            S.ActionPill { theme: root.theme; text: i18n("Take a screenshot…"); icon.name: "screenshot-fullscreen-symbolic"; onClicked: screenshotMaker.take() }
            C.Label { text: i18n("Name"); color: root.theme.textColor }
            S.TextFieldPill {
                id: nameField; objectName: "globalThemeSnapshotName"; theme: root.theme; Layout.fillWidth: true; Layout.maximumWidth: 520
                placeholderText: i18n("Type in name…"); Accessible.name: i18n("Theme name")
                validator: Private.LookAndFeelNameValidator { model: kcm.model }
            }
            C.Label { Layout.fillWidth: true; text: i18n("A global theme with this name or id already exists."); visible: nameField.text.length > 0 && !nameField.acceptableInput; color: root.theme.textColor; wrapMode: Text.WordWrap }
            S.ActionPill {
                objectName: "globalThemeSnapshotConfirm"; theme: root.theme; text: i18n("Confirm"); icon.name: "checkmark-symbolic"
                enabled: nameField.text.length > 0 && nameField.acceptableInput
                onClicked: { kcm.saveCurrentTheme(nameField.text,root.snapshotPreview); root.subpage = ""; root.flickable.contentY = 0; }
            }
        }
    }
    Private.ScreenshotMaker { id: screenshotMaker; onAccepted: fileUrl => root.snapshotPreview = fileUrl }
    Dialogs.FileDialog { id: imageChooser; title: i18n("Choose a Preview Image"); nameFilters: [Private.FileDialogNameFilters.imageFiles()]; onAccepted: root.snapshotPreview = selectedFile }
    S.SurfaceDialog {
        id: applySheet; objectName: "globalThemeConfirmSheet"
        theme: root.theme; backdropItem: root
        title: i18n("Apply %1?",selectedInfo.name)
        property bool detailed: false
        contentItem: C.ScrollView {
            id: optionsScroll
            implicitHeight: options.implicitHeight; contentWidth: availableWidth; clip: true
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            ColumnLayout {
                id: options; width: optionsScroll.availableWidth; spacing: 12
                C.Label { Layout.fillWidth: true; text: i18n("The following will be applied by this Global Theme:"); color: root.theme.textColor; wrapMode: Text.WordWrap }
                S.CheckPill {
                    theme: root.theme; Layout.fillWidth: true; visible: !applySheet.detailed && root.hasAppearance && root.hasLayout
                    text: i18n("Appearance settings"); checked: (kcm.selectedContents & Private.LookandFeelManager.AppearanceSettings) !== 0
                    onToggled: root.toggleContents(Private.LookandFeelManager.AppearanceSettings)
                }
                S.CheckPill {
                    objectName: "globalThemeLayoutToggle"; theme: root.theme; Layout.fillWidth: true
                    visible: !applySheet.detailed && root.hasAppearance && root.hasLayout && !kcm.plasmaLocked
                    text: i18n("Desktop and window layout"); checked: (kcm.selectedContents & Private.LookandFeelManager.LayoutSettings) !== 0
                    onToggled: root.toggleContents(Private.LookandFeelManager.LayoutSettings)
                }
                C.Label { Layout.fillWidth: true; visible: !applySheet.detailed && root.hasAppearance && !root.hasLayout; text: i18n("Appearance settings"); color: root.theme.textColor }
                C.Label { Layout.fillWidth: true; visible: !applySheet.detailed && root.hasLayout && !root.hasAppearance; text: i18n("Desktop and window layout"); color: root.theme.textColor }
                GridLayout {
                    visible: applySheet.detailed; Layout.fillWidth: true; columns: applySheet.width < 540 ? 1 : 2; columnSpacing: 10; rowSpacing: 10
                    Repeater {
                        model: [
                            {label:i18n("Desktop layout"),flag:Private.LookandFeelManager.DesktopLayout | Private.LookandFeelManager.WindowPlacement,allowed:!kcm.plasmaLocked},
                            {label:i18n("Titlebar Buttons layout"),flag:Private.LookandFeelManager.TitlebarLayout},
                            {label:i18n("Colors"),flag:Private.LookandFeelManager.Colors},
                            {label:i18n("Application Style"),flag:Private.LookandFeelManager.WidgetStyle},
                            {label:i18n("Window Decoration Style"),flag:Private.LookandFeelManager.WindowDecoration},
                            {label:i18n("Window Decoration Size"),flag:Private.LookandFeelManager.BorderSize},
                            {label:i18n("Icons"),flag:Private.LookandFeelManager.Icons},
                            {label:i18n("Plasma Style"),flag:Private.LookandFeelManager.PlasmaTheme},
                            {label:i18n("Pointers"),flag:Private.LookandFeelManager.Cursors},
                            {label:i18n("Fonts"),flag:Private.LookandFeelManager.Fonts},
                            {label:i18n("Task Switcher"),flag:Private.LookandFeelManager.WindowSwitcher},
                            {label:i18n("Splash Screen"),flag:Private.LookandFeelManager.SplashScreen}
                        ]
                        delegate: S.CheckPill {
                            required property var modelData
                            objectName: "globalThemeOption-" + modelData.flag
                            Layout.fillWidth: true; Layout.preferredWidth: 240; theme: root.theme; text: modelData.label
                            visible: (kcm.themeContents & modelData.flag) !== 0 && modelData.allowed !== false
                            checked: (kcm.selectedContents & modelData.flag) !== 0
                            onToggled: root.toggleContents(modelData.flag)
                        }
                    }
                }
                K.InlineMessage {
                    objectName: "globalThemeLayoutWarning"; Layout.fillWidth: true
                    visible: (kcm.selectedContents & Private.LookandFeelManager.DesktopLayout) !== 0
                    type: K.MessageType.Warning
                    text: i18n("Applying a Desktop layout will delete the current set of desktops, panels, docks, and widgets, replacing them with what the theme specifies.")
                }
                C.Label { Layout.fillWidth: true; visible: !root.hasAppearance && !root.hasLayout; color: root.theme.textColor; wrapMode: Text.WordWrap; text: i18n("This Global Theme does not provide any applicable settings. Please contact the maintainer of this Global Theme as it might be broken.") }
            }
        }
        footer: Item {
            implicitHeight: footerControls.implicitHeight + 24
            GridLayout {
                id: footerControls; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 12; y: 12
                columns: applySheet.width < 560 ? 2 : 3; columnSpacing: 8; rowSpacing: 8
                S.ActionPill {
                    objectName: "globalThemeMoreOptions"; theme: root.theme; Layout.columnSpan: applySheet.width < 560 ? 2 : 1
                    text: applySheet.detailed ? i18n("Show fewer options…") : i18n("Choose what to apply…"); enabled: root.hasAppearance
                    onClicked: { applySheet.detailed = !applySheet.detailed; kcm.selectedItems = undefined; }
                }
                S.ActionPill { objectName: "globalThemeConfirmCancel"; theme: root.theme; text: i18n("Cancel"); onClicked: applySheet.close() }
                S.ActionPill {
                    id: confirmApply; objectName: "globalThemeConfirmApply"; theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"
                    enabled: (kcm.selectedContents & (Private.LookandFeelManager.AppearanceSettings | Private.LookandFeelManager.LayoutSettings | Private.LookandFeelManager.DesktopLayout)) !== 0
                    onClicked: { kcm.apply(); applySheet.close(); themeGrid.forceActiveFocus(); }
                }
            }
        }
    }
    S.SurfaceDialog {
        id: deleteSheet; objectName: "globalThemeDeleteSheet"
        theme: root.theme; backdropItem: root; title: i18n("Delete Permanently")
        contentItem: C.Label { text: i18n("Do you really want to permanently delete this theme?"); wrapMode: Text.WordWrap; color: root.theme.textColor }
        footer: Item {
            implicitHeight: 66
            RowLayout {
                anchors.fill: parent; anchors.margins: 12
                Item { Layout.fillWidth: true }
                S.ActionPill { theme: root.theme; text: i18n("Cancel"); onClicked: deleteSheet.close() }
                S.ActionPill { objectName: "globalThemeDeleteConfirm"; theme: root.theme; text: i18n("Delete Permanently"); icon.name: "edit-delete"; onClicked: { kcm.removeRow(root.removalPackage,true); deleteSheet.close(); } }
            }
        }
    }
    Window {
        id: themePreview; objectName: "globalThemePreviewWindow"
        color: Qt.rgba(0,0,0,.85)
        S.RoundedPreview { id: previewImage; anchors.fill: parent; anchors.margins: 24; theme: root.theme; backgroundColor: "transparent"; radius: 16; sourceSize: Qt.size(width*Screen.devicePixelRatio,height*Screen.devicePixelRatio) }
        MouseArea { anchors.fill: parent; onClicked: themePreview.close() }
        S.CircleButton { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 20; theme: root.theme; label: i18n("Close preview"); icon.name: "window-close"; onClicked: themePreview.close() }
        Shortcut { sequence: "Esc"; onActivated: themePreview.close(); enabled: themePreview.visible }
    }
}
