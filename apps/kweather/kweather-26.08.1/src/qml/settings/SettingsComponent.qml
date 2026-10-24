/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2022 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: root

    property var dialog: null // dialog component if this is within a dialog

    signal closeRequested()

    spacing: 0

    component SettingsCard: Item {
        default property alias delegates: rows.data
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
        title: i18n("General")
    }

    SettingsCard {
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
        FormCard.FormComboBoxDelegate {
            id: forecastStyleDropdown
            objectName: "weatherForecastStyleSetting"
            background: RowBackground { control: forecastStyleDropdown }

            text: i18n("Forecast Style")
            currentIndex: indexOfValue(settingsModel.forecastStyle)
            textRole: "name"
            valueRole: "value"
            displayMode: FormCard.FormComboBoxDelegate.Dialog

            onActivated: settingsModel.save()
            onCurrentValueChanged: settingsModel.forecastStyle = currentValue

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
                    // indexOfValue doesn't bind to model changes unfortunately, set currentIndex manually here
                    forecastStyleDropdown.currentIndex = forecastStyleDropdown.indexOfValue(settingsModel.forecastStyle);
                }
            }

        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            id: aboutButton
            background: RowBackground { control: aboutButton }

            text: i18n("About")
            onClicked: {
                applicationWindow().pageStack.push(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage"));
                root.closeRequested();
            }
        }
    }

    FormCard.FormHeader {
        title: i18n("Units")
    }

    SettingsCard {
        TemperatureUnitComboBox {
            id: temperatureUnitsDropdown
            objectName: "weatherTemperatureSetting"
            background: RowBackground { control: temperatureUnitsDropdown }
        }

        FormCard.FormDelegateSeparator {}

        SpeedUnitComboBox {
            id: speedUnitsDropdown
            background: RowBackground { control: speedUnitsDropdown }
        }

        FormCard.FormDelegateSeparator {}

        PressureUnitComboBox {
            id: pressureUnitsDropdown
            background: RowBackground { control: pressureUnitsDropdown }
        }

        FormCard.FormDelegateSeparator {}

        // Precipitation
        PrecipitationUnitComboBox {
            id: precipitationUnitsDropdown
            background: RowBackground { control: precipitationUnitsDropdown }
        }
    }
}
