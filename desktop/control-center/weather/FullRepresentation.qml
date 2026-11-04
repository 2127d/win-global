// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
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
    property bool canConfigure: true
    signal configureRequested()
    signal sourceRequested(url source)
    readonly property bool refreshing: status === Weather.ForecastControl.Connecting
    readonly property bool ready: status === Weather.ForecastControl.Normal
        || (refreshing && (hasTemperature || (futureDays?.daysNumber ?? 0) > 0))
    readonly property bool hasTemperature: lastObservation?.temperature !== undefined
        && lastObservation.temperature !== null && !isNaN(lastObservation.temperature) && !!metaData?.temperatureUnit
    readonly property var feelsLike: [lastObservation?.windchill, lastObservation?.heatIndex, lastObservation?.humidex]
        .find(value => value !== undefined && value !== null && !isNaN(value))
    FontMetrics { id: sizingMetrics; font: Kirigami.Theme.defaultFont }
    readonly property real forecastLabelWidth: futureDays?.isNightPresent
        ? Math.max(sizingMetrics.advanceWidth(qsTr("Day")), sizingMetrics.advanceWidth(qsTr("Night"))) + 8 : 0
    readonly property real sequoiaPreferredWidth: Math.min(600, Math.max(480,
        24 + forecastLabelWidth + (futureDays?.daysNumber ?? 0) * Math.max(56, sizingMetrics.height * 3.1)))
    readonly property real sequoiaPreferredHeight: (ready ? header.implicitHeight + panels.naturalHeight : stateCard.implicitHeight)
        + footerArea.implicitHeight + 10
    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
    background: null
    header: PlasmaExtras.PlasmoidHeading {
        visible: root.ready
        background: Item {}
        leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
        leftPadding: 0; rightPadding: 0; topPadding: 4; bottomPadding: 8
        contentItem: CurrentWeatherCard { controller: root }
    }
    SwitchPanel {
        id: panels
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footerArea.top; bottomMargin: 10 }
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
    PC.ScrollView {
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footerArea.top; bottomMargin: 10 }
        visible: !root.ready
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentWidth: availableWidth
        WeatherCard {
            id: stateCard
            objectName: "weatherStateCard"
            width: parent.width
            implicitHeight: stateContent.implicitHeight + 40
            ColumnLayout {
                id: stateContent
                anchors.fill: parent; anchors.margins: 20; spacing: 12
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 56; Layout.preferredHeight: 56; radius: 18
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
                    Kirigami.Icon {
                        anchors.centerIn: parent; width: 28; height: 28
                        source: root.status === Weather.ForecastControl.NeedsConfiguration ? "mark-location" : "weather-none-available"
                    }
                }
                PC.BusyIndicator { Layout.alignment: Qt.AlignHCenter; visible: root.status === Weather.ForecastControl.Connecting; running: visible }
                PC.Label {
                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; font.weight: Font.DemiBold
                    text: root.status === Weather.ForecastControl.NeedsConfiguration ? qsTr("Choose your weather location")
                        : root.status === Weather.ForecastControl.Connecting ? qsTr("Updating weather…") : qsTr("Weather is temporarily unavailable")
                }
                PC.Label {
                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
                    color: Kirigami.Theme.disabledTextColor
                    text: root.status === Weather.ForecastControl.NeedsConfiguration ? qsTr("Choose a location and provider in Settings to see your forecast.")
                        : root.status === Weather.ForecastControl.Connecting ? qsTr("Getting the latest report from your weather provider.")
                        : qsTr("Your weather provider could not supply a report. KDE will try again at your configured update interval.")
                }
                WeatherFooterButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Set location…"); visible: root.status === Weather.ForecastControl.NeedsConfiguration && root.canConfigure
                    onClicked: root.configureRequested()
                }
            }
        }
    }
    ColumnLayout {
        id: footerArea
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        spacing: 6
        PC.Label {
            objectName: "weatherUpdatingLabel"
            Layout.alignment: Qt.AlignHCenter
            visible: root.ready && root.refreshing
            text: qsTr("Updating…")
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
        }
        PC.Label {
            Layout.fillWidth: true
            visible: root.ready && text.length > 0
            text: root.metaData?.credit || ""
            textFormat: Text.PlainText; color: Kirigami.Theme.disabledTextColor
            font: Kirigami.Theme.smallFont; wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
        WeatherCard {
            objectName: "weatherActionIsland"
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: actions.implicitWidth + 16
            Layout.maximumWidth: footerArea.width
            implicitHeight: actions.implicitHeight + 8
            radius: height / 2; elevated: true
            RowLayout {
                id: actions; anchors.centerIn: parent; spacing: 2
                WeatherFooterButton {
                    objectName: "weatherSettings"
                    flat: true; text: qsTr("Settings"); icon.name: "configure-symbolic"
                    visible: root.canConfigure; onClicked: root.configureRequested()
                }
                Rectangle { visible: root.canConfigure && sourceButton.visible; width: 1; height: 18; color: Qt.alpha(Kirigami.Theme.textColor, 0.15) }
                WeatherFooterButton {
                    id: sourceButton
                    objectName: "weatherSource"
                    flat: true; text: qsTr("Source"); icon.name: "window-new-symbolic"
                    visible: root.ready && !!root.metaData?.creditURL
                    onClicked: root.sourceRequested(root.metaData.creditURL)
                }
            }
        }
    }
}
