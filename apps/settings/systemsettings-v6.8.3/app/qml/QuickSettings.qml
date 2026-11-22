// SPDX-License-Identifier: GPL-2.0-or-later
// Presentation for KDE's native landing-page KCM; all edits use its skeleton.
import QtQuick
import "qrc:/qt/qml/org/kde/systemsettings" as Hub
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
import org.kde.kcmutils as KCMUtils
import org.kde.plasma.landingpage.kcm
import org.sequoia.components 1.0 as S

KCMUtils.SimpleKCM {
    id: root
    objectName: "quickSettingsHub"
    title: ""
    globalToolBarStyle: K.ApplicationHeaderStyle.None
    verticalScrollBarPolicy: C.ScrollBar.AlwaysOff
    implicitWidth: 820; implicitHeight: 680
    leftPadding: 14; rightPadding: 14; topPadding: 12; bottomPadding: 12
    readonly property real themeCardMaximumWidth: 360
    readonly property real shortcutMaximumWidth: 280
    readonly property real choiceCardMaximumWidth: 400
    readonly property real settingsMaximumWidth: choiceCardMaximumWidth * 2 + 12
    property var settings: kcm.globalsSettings
    property var backend: kcm
    property S.Theme theme: Hub.AppearanceLibraryTheme {}
    property Hub.ScrollEdgeFade edgeFade: Hub.ScrollEdgeFade { viewport: root.flickable; fadeSize: 24 }

    component Shortcut: S.ActionPill {
        id: shortcut
        Layout.maximumWidth: root.shortcutMaximumWidth
        property var nativeAction: null
        property var sourceIcon: nativeAction ? nativeAction.icon : ""
        theme: root.theme
        text: nativeAction ? nativeAction.text : ""
        enabled: !nativeAction || nativeAction.enabled
        contentItem: RowLayout {
            spacing: 8
            K.Icon { source: shortcut.sourceIcon; implicitWidth: 22; implicitHeight: 22 }
            C.Label { Layout.fillWidth: true; text: shortcut.text; color: root.theme.textColor; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter }
        }
        onClicked: if (nativeAction) nativeAction.trigger()
    }
    header: Item {
        implicitHeight: 70
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 12
            S.CircleButton {
                objectName: "quickSettingsBack"
                theme: root.theme; label: i18n("Back"); icon.name: "go-previous"
                // Leaving the native KCM destroys this page; defer until this handler returns.
                onClicked: Qt.callLater(navigation.back)
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                K.Heading { color: root.theme.textColor; text: i18n("Quick Settings"); level: 1; font.weight: Font.DemiBold }
                C.Label { text: i18n("Your everyday desktop preferences"); color: root.theme.mutedColor; Layout.fillWidth: true; elide: Text.ElideRight }
            }
        }
    }
    footer: Item {
        implicitHeight: 56
        RowLayout {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            S.ActionPill {
                theme: root.theme; text: i18n("Defaults"); icon.name: "edit-undo"
                visible: (kcm.buttons & KCMUtils.ConfigModule.Default) !== 0
                enabled: !kcm.representsDefaults
                onClicked: moduleActions.moduleDefaults()
            }
            Item { Layout.fillWidth: true }
            S.ActionPill {
                objectName: "quickSettingsReset"
                theme: root.theme; text: i18n("Reset"); icon.name: "edit-undo"
                enabled: kcm.needsSave
                onClicked: moduleActions.moduleLoad()
            }
            S.ActionPill {
                objectName: "quickSettingsApply"
                theme: root.theme; text: i18n("Apply"); icon.name: "checkmark-symbolic"
                enabled: kcm.needsSave
                onClicked: moduleActions.saveActiveModule()
            }
        }
    }
    ColumnLayout {
        spacing: 14
        K.Heading { color: root.theme.textColor; text: i18n("Theme"); level: 3; Layout.fillWidth: true; Layout.maximumWidth: themeOptions.width; Layout.alignment: Qt.AlignHCenter }
        GridLayout {
            id: themeOptions
            Layout.fillWidth: true; columns: root.width < 440 ? 1 : root.width < 680 ? 2 : 3
            Layout.maximumWidth: columns * root.themeCardMaximumWidth + (columns - 1) * columnSpacing
            Layout.alignment: Qt.AlignHCenter
            columnSpacing: 12; rowSpacing: 12
            C.ButtonGroup { id: themeGroup }
            Hub.QuickThemeCard {
                id: lightCard; objectName: "quickSettingsLight"
                Layout.fillWidth: true; Layout.preferredWidth: 220; Layout.maximumWidth: root.themeCardMaximumWidth
                theme: root.theme; backdropItem: root; text: i18n("Light")
                packageId: root.settings.defaultLightLookAndFeel; variant: LookAndFeel.Variant.Light
                C.ButtonGroup.group: themeGroup
                KCMUtils.SettingHighlighter { highlight: root.settings.automaticLookAndFeel || root.settings.lookAndFeelPackage !== kcm.defaultLookAndFeelPackage }
                checked: !root.settings.automaticLookAndFeel && root.settings.lookAndFeelPackage === packageId
                onToggled: { root.settings.automaticLookAndFeel = false; root.settings.lookAndFeelPackage = packageId; }
                onAccepted: packageId => root.settings.defaultLightLookAndFeel = packageId
            }
            Hub.QuickThemeCard {
                id: darkCard; objectName: "quickSettingsDark"
                Layout.fillWidth: true; Layout.preferredWidth: 220; Layout.maximumWidth: root.themeCardMaximumWidth
                theme: root.theme; backdropItem: root; text: i18n("Dark")
                packageId: root.settings.defaultDarkLookAndFeel; variant: LookAndFeel.Variant.Dark
                C.ButtonGroup.group: themeGroup
                KCMUtils.SettingHighlighter { highlight: root.settings.automaticLookAndFeel || root.settings.lookAndFeelPackage !== kcm.defaultLookAndFeelPackage }
                checked: !root.settings.automaticLookAndFeel && root.settings.lookAndFeelPackage === packageId
                onToggled: { root.settings.automaticLookAndFeel = false; root.settings.lookAndFeelPackage = packageId; }
                onAccepted: packageId => root.settings.defaultDarkLookAndFeel = packageId
            }
            S.ThemePreviewCard {
                objectName: "quickSettingsAutomatic"
                Layout.fillWidth: true; Layout.preferredWidth: 220; Layout.maximumWidth: root.themeCardMaximumWidth
                theme: root.theme; text: i18n("Automatic"); caption: i18n("Day and night"); changeEnabled: false
                previewSource: lightCard.previewSource; secondaryPreviewSource: darkCard.previewSource
                C.ButtonGroup.group: themeGroup
                KCMUtils.SettingHighlighter { highlight: root.settings.automaticLookAndFeel || root.settings.lookAndFeelPackage !== kcm.defaultLookAndFeelPackage }
                checked: root.settings.automaticLookAndFeel
                onToggled: root.settings.automaticLookAndFeel = true
                C.ToolTip.text: i18n("Use “%1” during the day and “%2” at night", lightCard.caption, darkCard.caption)
                C.ToolTip.visible: hovered; C.ToolTip.delay: K.Units.toolTipDelay
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Layout.maximumWidth: root.shortcutMaximumWidth * 2 + spacing; Layout.alignment: Qt.AlignHCenter
            Shortcut { objectName: "quickSettingsWallpaper"; Layout.fillWidth: true; nativeAction: kcm.kcmAction("kcm_wallpaper"); visible: nativeAction !== null }
            Shortcut { objectName: "quickSettingsGlobalTheme"; Layout.fillWidth: true; nativeAction: kcm.kcmAction("kcm_lookandfeel"); visible: nativeAction !== null }
        }
        K.Heading { color: root.theme.textColor; text: i18n("Animation speed"); level: 3; Layout.topMargin: 8; Layout.fillWidth: true; Layout.maximumWidth: root.settingsMaximumWidth; Layout.alignment: Qt.AlignHCenter }
        S.SliderPill {
            id: speed; objectName: "quickSettingsAnimation"
            Layout.fillWidth: true; Layout.maximumWidth: root.settingsMaximumWidth; Layout.alignment: Qt.AlignHCenter
            theme: root.theme; showTicks: true
            readonly property var valueMapping: [4, 2, 1.5, 1, 0.75, 0.5, 0]
            from: 0; to: 6; stepSize: 1; snapMode: C.Slider.SnapAlways
            value: { const index = valueMapping.findIndex(item => item <= root.settings.animationDurationFactor); return index >= 0 ? index : 6; }
            onMoved: root.settings.animationDurationFactor = valueMapping[value]
            Accessible.name: i18n("Animation speed")
            KCMUtils.SettingStateBinding { configObject: root.settings; settingName: "animationDurationFactor" }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.maximumWidth: root.settingsMaximumWidth; Layout.alignment: Qt.AlignHCenter
            C.Label { text: i18n("Slow"); color: root.theme.mutedColor }
            Item { Layout.fillWidth: true }
            C.Label { text: i18n("Instant"); color: root.theme.mutedColor }
        }
        K.Heading { color: root.theme.textColor; text: i18n("Clicking files or folders"); level: 3; Layout.topMargin: 8; Layout.fillWidth: true; Layout.maximumWidth: clickOptions.width; Layout.alignment: Qt.AlignHCenter }
        GridLayout {
            id: clickOptions
            Layout.fillWidth: true; columns: root.width < 580 ? 1 : 2; columnSpacing: 12; rowSpacing: 12
            Layout.maximumWidth: columns * root.choiceCardMaximumWidth + (columns - 1) * columnSpacing
            Layout.alignment: Qt.AlignHCenter
            C.ButtonGroup { id: clickGroup }
            S.ChoiceCard {
                objectName: "quickSettingsDoubleClick"
                Layout.fillWidth: true; Layout.preferredWidth: 260; Layout.maximumWidth: root.choiceCardMaximumWidth; theme: root.theme
                text: i18n("Selects them"); description: i18n("Open by double-clicking instead")
                checked: !root.settings.singleClick; C.ButtonGroup.group: clickGroup
                onToggled: root.settings.singleClick = false
                KCMUtils.SettingStateBinding { configObject: root.settings; settingName: "singleClick" }
            }
            S.ChoiceCard {
                objectName: "quickSettingsSingleClick"
                Layout.fillWidth: true; Layout.preferredWidth: 260; Layout.maximumWidth: root.choiceCardMaximumWidth; theme: root.theme
                text: i18n("Opens them"); description: i18n("Select by clicking on item's selection marker")
                checked: root.settings.singleClick; C.ButtonGroup.group: clickGroup
                onToggled: root.settings.singleClick = true
                KCMUtils.SettingStateBinding { configObject: root.settings; settingName: "singleClick" }
            }
        }
        Shortcut { Layout.alignment: Qt.AlignHCenter; nativeAction: kcm.kcmAction("kcm_workspace"); visible: nativeAction !== null }
        K.Heading { color: root.theme.textColor; text: i18n("Most used pages"); level: 3; visible: recent.count > 0; Layout.topMargin: 8; Layout.fillWidth: true; Layout.maximumWidth: recentOptions.width; Layout.alignment: Qt.AlignHCenter }
        GridLayout {
            id: recentOptions
            Layout.fillWidth: true; columns: root.width < 580 ? 1 : 2; columnSpacing: 10; rowSpacing: 10
            Layout.maximumWidth: columns * root.shortcutMaximumWidth + (columns - 1) * columnSpacing
            Layout.alignment: Qt.AlignHCenter
            visible: recent.count > 0
            Repeater {
                id: recent; model: kcm.mostUsedModel
                delegate: Shortcut {
                    required property var model
                    objectName: "quickSettingsRecent-" + model.kcmPlugin
                    Layout.fillWidth: true; Layout.preferredWidth: 260
                    text: model.display; sourceIcon: model.decoration
                    onClicked: kcm.openKCM(model.kcmPlugin)
                }
            }
        }
    }
}
