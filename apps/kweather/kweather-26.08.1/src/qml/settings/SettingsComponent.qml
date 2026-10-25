/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2022 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import "qrc:/sequoia-weather/ActionIcons.js" as ActionIcons
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: root

    property var dialog: null // dialog component if this is within a dialog
    property var viewport: null
    property var activeChoice: null
    readonly property real collapsedHeight: generalHeader.implicitHeight + unitsHeader.implicitHeight + 16 + generalCard.collapsedHeight + unitsCard.collapsedHeight
    function openChoice(choice) {
        if (activeChoice && activeChoice !== choice) activeChoice.expanded = false;
        activeChoice = choice;
    }

    signal closeRequested()
    signal aboutRequested()
    function focusAbout() { aboutButton.forceActiveFocus(); }

    spacing: 0

    component SettingsCard: Item {
        default property alias delegates: rows.data
        readonly property real collapsedHeight: rows.children.reduce((height, child) => height + (child.collapsedHeight ?? child.implicitHeight), 18)
        Layout.fillWidth: true
        implicitHeight: rows.implicitHeight + 18
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false
        Kirigami.ShadowedRectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(540, parent.width - 32)
            height: parent.height
            radius: 22
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.13)
            shadow.size: Kirigami.Units.largeSpacing
            shadow.color: Qt.alpha(Kirigami.Theme.textColor, 0.10)
            ColumnLayout {
                id: rows
                anchors.fill: parent; anchors.margins: 9
                spacing: 0
            }
        }
    }

    component RowBackground: Rectangle {
        required property var control
        radius: 13
        color: control.down ? Qt.alpha(Kirigami.Theme.textColor, 0.12)
            : control.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.05) : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Kirigami.Theme.highlightColor
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }

    FormCard.FormHeader {
        id: generalHeader
        title: i18n("General")
        Layout.bottomMargin: 8
    }

    SettingsCard {
        id: generalCard
        SwitchDelegate {
            id: motionSwitch
            objectName: "weatherAnimateScenery"
            Layout.fillWidth: true
            text: i18n("Animate weather scenery")
            Accessible.description: i18n("Subtle motion in Dynamic view. Turn off for a still sky.")
            implicitHeight: Math.max(motionLabels.implicitHeight, indicator.implicitHeight) + 28
            Layout.minimumHeight: implicitHeight
            leftPadding: mirrored ? indicator.width + 24 : 12
            rightPadding: mirrored ? 12 : indicator.width + 24
            topPadding: 14; bottomPadding: 14
            hoverEnabled: true
            checked: settingsModel.animateWeather
            onToggled: { settingsModel.animateWeather = checked; settingsModel.save(); }
            background: RowBackground { control: motionSwitch }
            indicator: SequoiaSwitchIndicator {
                control: motionSwitch
                x: motionSwitch.mirrored ? 12 : motionSwitch.width - width - 12
                y: (motionSwitch.height - height) / 2
            }
            contentItem: ColumnLayout {
                id: motionLabels
                spacing: 5
                Label { text: motionSwitch.text; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Label {
                    text: motionSwitch.Accessible.description
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                    color: Kirigami.Theme.disabledTextColor
                }
            }
        }
        InlineChoice {
            id: forecastStyleDropdown
            viewport: root.viewport
            objectName: "weatherForecastStyleSetting"
            onExpandedChanged: if (expanded) root.openChoice(forecastStyleDropdown)

            text: i18n("Forecast Style")
            selectedValue: settingsModel.forecastStyle
            textRole: "name"
            valueRole: "value"

            onActivated: index => { settingsModel.forecastStyle = model.get(index).value; settingsModel.save(); }

            model: ListModel {
                // we can't use i18n with ListElement
                Component.onCompleted: {
                    append({
                        "name": i18n("Flat"),
                        "value": "Flat"
                    });
                    append({
                        "name": i18n("Dynamic"),
                        "value": "Dynamic"
                    });
                }
            }

        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            id: aboutButton
            objectName: "weatherOpenAbout"
            background: RowBackground { control: aboutButton }

            text: i18n("About")
            contentItem: RowLayout {
                spacing: 12
                Label { text: aboutButton.text; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Kirigami.Icon {
                    source: ActionIcons.source(aboutButton.mirrored ? "go-previous-symbolic" : "go-next-symbolic")
                    isMask: true
                    Layout.preferredWidth: 18; Layout.preferredHeight: 18
                }
            }
            onClicked: root.aboutRequested()
        }
    }

    FormCard.FormHeader {
        id: unitsHeader
        title: i18n("Units")
        Layout.bottomMargin: 8
    }

    SettingsCard {
        id: unitsCard
        TemperatureUnitComboBox {
            id: temperatureUnitsDropdown
            viewport: root.viewport
            objectName: "weatherTemperatureSetting"
            onExpandedChanged: if (expanded) root.openChoice(temperatureUnitsDropdown)
        }

        FormCard.FormDelegateSeparator {}

        SpeedUnitComboBox {
            id: speedUnitsDropdown
            viewport: root.viewport
            objectName: "weatherSpeedSetting"
            onExpandedChanged: if (expanded) root.openChoice(speedUnitsDropdown)
        }

        FormCard.FormDelegateSeparator {}

        PressureUnitComboBox {
            id: pressureUnitsDropdown
            viewport: root.viewport
            objectName: "weatherPressureSetting"
            onExpandedChanged: if (expanded) root.openChoice(pressureUnitsDropdown)
        }

        FormCard.FormDelegateSeparator {}

        // Precipitation
        PrecipitationUnitComboBox {
            id: precipitationUnitsDropdown
            viewport: root.viewport
            objectName: "weatherPrecipitationSetting"
            onExpandedChanged: if (expanded) root.openChoice(precipitationUnitsDropdown)
        }
    }
}
