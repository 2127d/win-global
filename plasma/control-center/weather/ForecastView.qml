// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.core as PlasmaCore
import plasma.applet.org.kde.plasma.weather as Weather

Column {
    id: root
    property var futureDays: null
    property var metaData: null
    property int displayTemperatureUnit: 0
    readonly property bool hasNight: futureDays?.isNightPresent ?? false
    readonly property int dayCount: futureDays?.daysNumber ?? 0
    spacing: 12
    FontMetrics { id: metrics; font: Kirigami.Theme.defaultFont }

    Repeater {
        model: root.dayCount > 0 ? (root.hasNight ? 2 : 1) : 0
        delegate: WeatherCard {
            id: card
            required property int index
            objectName: index === 1 ? "weatherNightCard" : "weatherDayCard"
            width: root.width
            implicitHeight: heading.height + daysHeader.height + forecast.rowHeight + 40
            readonly property real cellWidth: Math.max(58, (width - 28) / Math.max(1, root.dayCount))
            ColumnLayout {
                id: heading
                x: 16; y: 14
                width: parent.width - 32
                RowLayout {
                    spacing: 7
                    Kirigami.Icon {
                        source: card.index === 1 ? "weather-clear-night" : "weather-clear"
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                    }
                    PC.Label {
                        text: root.hasNight ? (card.index === 1 ? qsTr("Night") : qsTr("Day")) : qsTr("Forecast")
                        font.weight: Font.DemiBold
                    }
                }
            }
            QQC2.HorizontalHeaderView {
                id: daysHeader
                x: 14; y: heading.y + heading.height + 10
                width: parent.width - 28
                height: metrics.height + 4
                syncView: forecast
                textRole: "monthDay"
                model: root.futureDays
                clip: true
                interactive: false
                resizableColumns: false
                delegate: Item {
                    required property var model
                    implicitWidth: card.cellWidth
                    implicitHeight: metrics.height + 4
                    PC.Label {
                        anchors.fill: parent
                        text: parent.model.monthDay ?? parent.model.weekDay ?? ""
                        textFormat: Text.PlainText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: Kirigami.Theme.disabledTextColor
                        font: Kirigami.Theme.smallFont
                    }
                }
            }
            TableView {
                id: forecast
                objectName: "weatherForecastTable" + card.index
                x: 14; y: daysHeader.y + daysHeader.height + 4
                width: parent.width - 28
                height: rowHeight
                clip: true
                model: root.futureDays
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick
                readonly property real rowHeight: 42 + metrics.height * (root.hasNight ? 2 : 3) + 16
                columnWidthProvider: column => card.cellWidth
                rowHeightProvider: row => row === card.index ? rowHeight : 0
                onWidthChanged: forceLayout()
                onModelChanged: forceLayout()
                delegate: Item {
                    id: cell
                    required property var model
                    implicitWidth: card.cellWidth
                    implicitHeight: forecast.rowHeight
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 3
                        opacity: cell.model.conditionIcon ? 1 : 0.45
                        Kirigami.Icon {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 38; Layout.preferredHeight: 38
                            source: cell.model.conditionIcon || Weather.Util.unknownWeatherIcon
                            PlasmaCore.ToolTipArea {
                                anchors.fill: parent
                                mainText: cell.model.condition || qsTr("No forecast available")
                            }
                        }
                        PC.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: cell.model.conditionProbability !== undefined && cell.model.conditionProbability !== "" && !isNaN(cell.model.conditionProbability)
                                ? qsTr("%1%").arg(cell.model.conditionProbability) : "—"
                            textFormat: Text.PlainText
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            visible: root.futureDays?.hasProbability ?? false
                            Accessible.name: qsTr("Precipitation probability: %1").arg(text)
                        }
                        PC.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.temperature(cell.model.highTemp)
                            font.weight: Font.DemiBold
                            visible: !isNaN(cell.model.highTemp) || !root.hasNight
                            textFormat: Text.PlainText
                        }
                        PC.Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.temperature(cell.model.lowTemp)
                            font.weight: root.hasNight ? Font.DemiBold : Font.Normal
                            color: root.hasNight ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                            visible: !isNaN(cell.model.lowTemp) || !root.hasNight
                            textFormat: Text.PlainText
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
    function temperature(value) {
        return value !== null && value !== undefined && value !== "" && !isNaN(value) && metaData?.temperatureUnit
            ? Weather.Util.temperatureToDisplayString(displayTemperatureUnit, value, metaData.temperatureUnit, true) : "—";
    }
}
