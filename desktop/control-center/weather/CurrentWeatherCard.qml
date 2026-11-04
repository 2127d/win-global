// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import plasma.applet.org.kde.plasma.weather as Weather

WeatherCard {
    id: card
    objectName: "weatherCurrentCard"
    required property var controller
    readonly property bool hasWind: controller.lastObservation?.windSpeed !== undefined
        && controller.lastObservation.windSpeed !== null && !isNaN(controller.lastObservation.windSpeed) && !!controller.metaData?.windSpeedUnit
    readonly property bool hasHumidity: controller.lastObservation?.humidity !== undefined
        && controller.lastObservation.humidity !== null && !isNaN(controller.lastObservation.humidity)
    implicitHeight: content.implicitHeight + 24
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 12; spacing: 8
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            ColumnLayout {
                Layout.fillWidth: true; spacing: 5
                PC.Label {
                    Layout.fillWidth: true
                    text: card.controller.station?.place || qsTr("Weather")
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText; wrapMode: Text.Wrap
                    maximumLineCount: 3; elide: Text.ElideRight
                }
                PC.Label {
                    objectName: "weatherTemperature"
                    Layout.fillWidth: true
                    text: card.controller.hasTemperature ? Weather.Util.temperatureToDisplayString(card.controller.displayTemperatureUnit,
                        card.controller.lastObservation.temperature, card.controller.metaData.temperatureUnit, true, false) : "—"
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 2.3
                    font.weight: Font.Light
                    textFormat: Text.PlainText
                }
                PC.Label {
                    Layout.fillWidth: true
                    text: card.controller.lastObservation?.currentConditions || qsTr("Current conditions")
                    textFormat: Text.PlainText; wrapMode: Text.Wrap
                }
                PC.Label {
                    Layout.fillWidth: true
                    visible: card.controller.hasTemperature && card.controller.feelsLike !== undefined
                        && card.controller.feelsLike !== card.controller.lastObservation.temperature
                    text: visible ? qsTr("Feels like %1").arg(Weather.Util.temperatureToDisplayString(card.controller.displayTemperatureUnit,
                        card.controller.feelsLike, card.controller.metaData.temperatureUnit, true, false)) : ""
                    color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap
                }
            }
            Rectangle {
                Layout.preferredWidth: 56; Layout.preferredHeight: 56
                radius: 18; color: Qt.alpha(Kirigami.Theme.highlightColor, 0.08)
                Kirigami.Icon {
                    anchors.centerIn: parent; width: 40; height: 40
                    source: card.controller.lastObservation?.conditionIcon || card.controller.futureDays?.firstDayIcon || Weather.Util.unknownWeatherIcon
                    fallback: Weather.Util.unknownWeatherIcon
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            visible: card.hasWind || card.hasHumidity
            PC.Label {
                Layout.fillWidth: true; visible: card.hasWind
                text: card.hasWind ? qsTr("Wind · %1").arg(card.controller.lastObservation.windSpeed === 0 ? qsTr("Calm")
                    : Weather.Util.valueToDisplayString(card.controller.displaySpeedUnit, card.controller.lastObservation.windSpeed, card.controller.metaData.windSpeedUnit, 1)) : ""
                color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap; textFormat: Text.PlainText
                font: Kirigami.Theme.smallFont
            }
            PC.Label {
                Layout.fillWidth: true; visible: card.hasHumidity
                text: card.hasHumidity ? qsTr("Humidity · %1").arg(Weather.Util.percentToDisplayString(card.controller.lastObservation.humidity)) : ""
                color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap; textFormat: Text.PlainText
                font: Kirigami.Theme.smallFont
                horizontalAlignment: card.hasWind ? Text.AlignRight : Text.AlignLeft
            }
        }
    }
}
