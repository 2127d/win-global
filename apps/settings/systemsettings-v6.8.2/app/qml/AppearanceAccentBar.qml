// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.sequoia.components 1.0 as S

HubPanel {
    id: bar
    required property Item backdropItem
    implicitWidth: swatches.implicitWidth + customButton.implicitWidth + moreColors.implicitWidth + 24 + 16
    implicitHeight: 58; radius: height / 2; tint: .42
    AppearanceColorDialog {
        id: customColorDialog
        objectName: "appearanceCustomAccentDialog"
        backdropItem: bar.backdropItem
    }
    RowLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 12
        C.ScrollView {
            id: swatchScroll
            property ScrollEdgeFade edgeFade: ScrollEdgeFade {
                objectName: "appearanceAccentFade"
                viewport: swatchScroll.contentItem
                fadeSize: 16
            }
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0
            Layout.preferredWidth: swatches.implicitWidth; Layout.maximumWidth: swatches.implicitWidth
            padding: 0; background: null; clip: true; contentHeight: availableHeight
            C.ScrollBar.vertical.policy: C.ScrollBar.AlwaysOff
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            Row {
                id: swatches
                spacing: 6
                Repeater {
                    model: [
                        {name:i18n("Blue"),color:"#3daee9"}, {name:i18n("Purple"),color:"#9b59b6"},
                        {name:i18n("Pink"),color:"#e56b9f"}, {name:i18n("Red"),color:"#da4453"},
                        {name:i18n("Orange"),color:"#f47750"}, {name:i18n("Gold"),color:"#fdbc4b"},
                        {name:i18n("Green"),color:"#27ae60"}, {name:i18n("Teal"),color:"#1abc9c"},
                        {name:i18n("Gray"),color:"#7f8c8d"}
                    ]
                    C.AbstractButton {
                        id: swatch
                        required property var modelData
                        objectName: "appearanceAccent-" + modelData.color.slice(1)
                        width: 42; height: 42; hoverEnabled: true; focusPolicy: Qt.StrongFocus
                        readonly property color swatchColor: modelData.color
                        readonly property bool selected: String(appearancePreview.selectedAccentColor).toLowerCase() === modelData.color
                        Accessible.name: modelData.name
                        Accessible.role: Accessible.RadioButton; Accessible.checked: selected
                        background: S.PillHighlight { theme: AppearanceLibraryTheme {}
 highlighted: swatch.selected; hovered: swatch.hovered; focused: swatch.visualFocus }
                        contentItem: Item {
                            Rectangle { anchors.centerIn: parent; width: 24; height: 24; radius: 12; color: swatch.modelData.color; border.width: 1; border.color: "#60ffffff" }
                            C.Label { anchors.centerIn: parent; text: "✓"; visible: swatch.selected; color: swatch.swatchColor.hslLightness > .6 ? "#202124" : "white"; font.bold: true }
                        }
                        C.ToolTip.visible: hovered || visualFocus
                        C.ToolTip.text: modelData.name
                        onActiveFocusChanged: if (activeFocus) {
                            const view = swatchScroll.contentItem;
                            if (x < view.contentX) view.contentX = x;
                            else if (x + width > view.contentX + swatchScroll.availableWidth) view.contentX = x + width - swatchScroll.availableWidth;
                        }
                        onClicked: appearancePreview.stageAccent(modelData.color)
                    }
                }
            }
        }
        S.ActionPill {
            id: customButton
            objectName: "appearanceCustomAccent"
            property alias dialog: customColorDialog
            theme: AppearanceLibraryTheme {}
            text: i18n("Custom color…")
            Layout.preferredHeight: 42
            contentItem: RowLayout {
                spacing: customButton.spacing
                opacity: customButton.enabled ? 1 : .45
                Rectangle {
                    implicitWidth: 18; implicitHeight: 18; radius: 9
                    color: appearancePreview.selectedAccentColor
                    border.width: 1; border.color: Qt.alpha(customButton.theme.textColor, .3)
                }
                C.Label { text: customButton.text; color: customButton.theme.textColor; font: customButton.font }
            }
            C.ToolTip.visible: hovered || visualFocus
            C.ToolTip.text: i18n("Selected accent: %1", String(appearancePreview.selectedAccentColor))
            onClicked: {
                customColorDialog.selectedColor = appearancePreview.selectedAccentColor;
                customColorDialog.open();
            }
        }
        S.CircleButton {
            id: moreColors
            theme: AppearanceLibraryTheme {}
 label: i18n("More colors"); ellipsis: true
            onClicked: navigation.open("kcm_colors")
        }
    }
}
