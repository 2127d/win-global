// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.sequoia.components 1.0 as S

ColumnLayout {
    id: controls
    required property Item backdropItem
    property S.Theme theme: AppearanceLibraryTheme {}
    readonly property var panel: panelSettings.selected
    readonly property bool hasPanel: panel.id !== undefined
    readonly property bool vertical: panel.location === "left" || panel.location === "right"
    readonly property real maximumContentWidth: grid.maximumColumns * 220 + (grid.maximumColumns - 1) * grid.columnSpacing
    spacing: 8
    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: selector.implicitWidth + refreshButton.implicitWidth + spacing
        Layout.alignment: Qt.AlignLeft
        S.ThemeSelector {
            id: selector
            objectName:"appearancePanelSelector"
            Layout.fillWidth:true; Layout.minimumWidth:0
            theme:controls.theme; backdropItem:controls.backdropItem
            Accessible.name:i18n("Select panel")
            model:panelSettings.panels
            currentIndex:model.findIndex(p => p.id === panelSettings.selectedId)
            enabled:model.length > 0
            displayText:controls.hasPanel ? currentText : i18n("No panels available")
            onActivated:function(index) { panelSettings.selectedId=model[index].id; }
        }
        S.CircleButton {
            id: refreshButton
            theme:controls.theme; label:i18n("Refresh panels"); icon.name:"view-refresh"
            enabled:!panelSettings.busy
            onClicked:panelSettings.refresh()
        }
    }

    C.Label {
        Layout.fillWidth:true; wrapMode:Text.WordWrap
        visible:!controls.hasPanel || panelSettings.error.length > 0 || controls.panel.locked === true
        text:panelSettings.error || (controls.panel.locked ? i18n("Unlock widgets to edit this panel.") : i18n("Create a panel from the desktop’s Edit Mode to get started."))
    }
    S.SettingsGrid {
        id: grid
        objectName:"appearancePanelSettingsGrid"
        visible:controls.hasPanel
        enabled:!controls.panel.locked
        Layout.fillWidth:true; Layout.fillHeight:false
        theme:controls.theme; backdropItem:controls.backdropItem
        maximumColumns:6; minimumCellWidth:160
        model:!controls.hasPanel ? [] : [
            {key:"location", label:i18n("Position"), type:"choice", value:controls.panel.location,
                options:[{name:i18n("Top"),value:"top"},{name:i18n("Bottom"),value:"bottom"},{name:i18n("Left"),value:"left"},{name:i18n("Right"),value:"right"}]},
            {key:"screen", label:i18n("Screen"), type:"choice", value:controls.panel.screen, options:panelSettings.screens, enabled:panelSettings.screens.length > 1},
            {key:"alignment", label:i18n("Alignment"), type:"choice", value:controls.panel.alignment,
                options:[{name:controls.vertical ? i18n("Top") : i18n("Left"),value:"left"},{name:i18n("Center"),value:"center"},{name:controls.vertical ? i18n("Bottom") : i18n("Right"),value:"right"}]},
            {key:"lengthMode", label:controls.vertical ? i18n("Height") : i18n("Width"), type:"choice", value:controls.panel.lengthMode,
                options:[{name:controls.vertical ? i18n("Fill height") : i18n("Fill width"),value:"fill"},{name:i18n("Fit content"),value:"fit"},{name:i18n("Custom"),value:"custom"}]},
            {key:"hiding", label:i18n("Visibility"), type:"choice", value:controls.panel.hiding,
                options:[{name:i18n("Always visible"),value:"none"},{name:i18n("Auto hide"),value:"autohide"},{name:i18n("Dodge windows"),value:"dodgewindows"},{name:i18n("Windows go below"),value:"windowsgobelow"}]},
            {key:"opacity", label:i18n("Opacity"), type:"choice", value:controls.panel.opacity, enabled:panelSettings.opacitySupported,
                description:panelSettings.opacitySupported ? "" : i18n("Not supported by this Plasma style"),
                options:[{name:i18n("Adaptive"),value:"adaptive"},{name:i18n("Opaque"),value:"opaque"},{name:i18n("Translucent"),value:"translucent"}]},
            {key:"floatingMode", label:i18n("Floating"), type:"choice", value:controls.panel.floatingMode,
                options:[{name:i18n("Disabled"),value:"disabled"},{name:i18n("Applets only"),value:"applets"},{name:i18n("Panel and applets"),value:"panel"}]},
            {key:"height", label:controls.vertical ? i18n("Panel width") : i18n("Panel height"), type:"range", value:controls.panel.height, minimum:20, maximum:controls.panel.maxThickness, step:2, suffix:i18n(" px")},
            {key:"minimumLength", label:i18n("Minimum length"), type:"range", value:controls.panel.minimumLength, minimum:10, maximum:controls.panel.side-controls.panel.offset, step:1, suffix:i18n(" px"), enabled:controls.panel.lengthMode === "custom"},
            {key:"maximumLength", label:i18n("Maximum length"), type:"range", value:controls.panel.maximumLength, minimum:10, maximum:controls.panel.side-controls.panel.offset, step:1, suffix:i18n(" px"), enabled:controls.panel.lengthMode === "custom"},
            {key:"offset", label:i18n("Offset"), type:"range", value:controls.panel.offset, minimum:0, maximum:Math.max(0,controls.panel.side-controls.panel.minimumLength), step:1, suffix:i18n(" px"), enabled:controls.panel.lengthMode === "custom"},
            {key:"edit", label:i18n("Widgets & more"), type:"action", actionText:i18n("Edit panel…"), description:i18n("Widgets, focus shortcut, clone or remove")}
        ]
        onSettingEdited:function(key,value) { panelSettings.setSetting(key,value); }
        onActionRequested:panelSettings.editPanel()
    }
    C.Label {
        visible:controls.hasPanel; Layout.fillWidth:true; wrapMode:Text.WordWrap
        text:i18n("Changes apply immediately to the selected panel.")
        color:controls.theme.mutedColor; font.pixelSize:11
    }
}
