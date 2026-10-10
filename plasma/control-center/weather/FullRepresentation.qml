// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid
import plasma.applet.org.kde.plasma.weather as Weather

PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaWeatherPage"
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    property int status: 0
    property int invalidUnit: 0
    property int displaySpeedUnit: 0
    property int displayPressureUnit: 0
    property int displayTemperatureUnit: 0
    property int displayVisibilityUnit: 0
    property var station: null
    property var futureDays: null
    property var warnings: null
    property var lastObservation: null
    property var metaData: null
    readonly property bool ready: status === Weather.ForecastControl.Normal
    readonly property bool hasTemperature: lastObservation?.temperature !== undefined
        && lastObservation.temperature !== null && !isNaN(lastObservation.temperature) && !!metaData?.temperatureUnit
    readonly property var feelsLike: [lastObservation?.windchill, lastObservation?.heatIndex, lastObservation?.humidex]
        .find(value => value !== undefined && value !== null && !isNaN(value))
    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
    background: null
    contentItem: PC.ScrollView {
        id: scroll
        PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 12
            WeatherCard {
                id: summary
                objectName: "weatherCurrentCard"
                Layout.fillWidth: true
                implicitHeight: current.implicitHeight + 32
                visible: root.ready
                RowLayout {
                    id: current
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        PC.Label {
                            Layout.fillWidth: true
                            text: root.station?.place || qsTr("Weather")
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            font.weight: Font.DemiBold
                        }
                        PC.Label {
                            visible: root.hasTemperature
                            text: root.hasTemperature ? Weather.Util.temperatureToDisplayString(root.displayTemperatureUnit,
                                root.lastObservation.temperature, root.metaData.temperatureUnit, true, false) : ""
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 2.4
                            font.weight: Font.Light
                        }
                        PC.Label {
                            Layout.fillWidth: true
                            visible: root.hasTemperature && root.feelsLike !== undefined && root.feelsLike !== null
                                && !isNaN(root.feelsLike) && root.feelsLike !== root.lastObservation.temperature
                            text: visible ? qsTr("Feels like %1").arg(Weather.Util.temperatureToDisplayString(
                                root.displayTemperatureUnit, root.feelsLike, root.metaData.temperatureUnit, true, false)) : ""
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.Wrap
                        }
                        PC.Label {
                            Layout.fillWidth: true
                            text: root.lastObservation?.currentConditions || qsTr("Forecast overview")
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }
                    Kirigami.Icon {
                        Layout.preferredWidth: 68; Layout.preferredHeight: 68
                        source: root.lastObservation?.conditionIcon || root.futureDays?.firstDayIcon || Weather.Util.unknownWeatherIcon
                        fallback: Weather.Util.unknownWeatherIcon
                    }
                }
            }
            SwitchPanel {
                Layout.fillWidth: true
                visible: root.ready
                futureDays: root.futureDays
                warnings: root.warnings
                lastObservation: root.lastObservation
                metaData: root.metaData
                invalidUnit: root.invalidUnit
                displayPressureUnit: root.displayPressureUnit
                displaySpeedUnit: root.displaySpeedUnit
                displayVisibilityUnit: root.displayVisibilityUnit
                displayTemperatureUnit: root.displayTemperatureUnit
            }
            WeatherCard {
                Layout.fillWidth: true
                implicitHeight: stateContent.implicitHeight + 48
                visible: !root.ready
                ColumnLayout {
                    id: stateContent
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14
                    Kirigami.Icon {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 40; Layout.preferredHeight: 40
                        source: root.status === Weather.ForecastControl.NeedsConfiguration ? "mark-location" : "weather-none-available"
                    }
                    PC.BusyIndicator {
                        Layout.alignment: Qt.AlignHCenter
                        visible: root.status === Weather.ForecastControl.Connecting
                        running: visible
                    }
                    PC.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: root.status === Weather.ForecastControl.NeedsConfiguration ? qsTr("Choose your weather location")
                            : root.status === Weather.ForecastControl.Connecting ? qsTr("Updating weather…") : qsTr("Weather is temporarily unavailable")
                        font.weight: Font.DemiBold
                    }
                    PC.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        color: Kirigami.Theme.disabledTextColor
                        text: qsTr("Weather information is provided by your selected weather service.")
                    }
                    PC.Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Set location…")
                        visible: root.status === Weather.ForecastControl.NeedsConfiguration
                        onClicked: Plasmoid.internalAction("configure").trigger()
                    }
                }
            }
            PC.Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 6
                visible: root.ready && text.length > 0
                text: root.metaData?.credit || ""
                textFormat: Text.PlainText
                color: Kirigami.Theme.disabledTextColor
                font: Kirigami.Theme.smallFont
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                TapHandler {
                    enabled: !!root.metaData?.creditURL
                    onTapped: Qt.openUrlExternally(root.metaData.creditURL)
                }
            }
        }
    }
}
