// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.core as PlasmaCore
import plasma.applet.org.kde.plasma.weather as Weather
import "../details" as Details

WeatherCard {
    id: root
    objectName: "weatherForecastCard"
    property var futureDays: null
    property var metaData: null
    property int displayTemperatureUnit: 0
    property int selectedDay: 0
    signal daySelected(int index)
    property int motionDuration: Kirigami.Units.longDuration
    readonly property bool hasNight: futureDays?.isNightPresent ?? false
    readonly property int dayCount: futureDays?.daysNumber ?? 0
    readonly property int periodCount: dayCount > 0 ? (hasNight ? 2 : 1) : 0
    readonly property real periodHeight: Math.max(84, metrics.height * 4 + 22)
    implicitHeight: heading.implicitHeight + dayHeader.height + periodCount * periodHeight + 42
    FontMetrics { id: metrics; font: Kirigami.Theme.defaultFont }

    function selectDay(index) {
        selectedDay = Math.max(0, Math.min(dayCount - 1, index));
        daySelected(selectedDay);
    }
    function alignDay(animate) {
        daySlide.stop();
        const destination = selectedDay * forecast.width;
        if (animate && motionDuration > 0 && Math.abs(forecast.contentX - destination) > 0.5) {
            daySlide.from = forecast.contentX;
            daySlide.to = destination;
            daySlide.start();
        } else {
            forecast.contentX = destination;
        }
        if (dayCount > 0) dayHeader.positionViewAtColumn(selectedDay, TableView.Contain);
    }
    onSelectedDayChanged: alignDay(true)
    onDayCountChanged: {
        selectDay(selectedDay);
        realign.restart();
    }
    Keys.onRightPressed: selectDay(selectedDay + 1)
    Keys.onLeftPressed: selectDay(selectedDay - 1)
    Timer { id: realign; interval: 0; onTriggered: root.alignDay(false) }

    RowLayout {
        id: heading
        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 12 }
        PC.Label { Layout.fillWidth: true; text: qsTr("Forecast"); font.weight: Font.DemiBold }
        PC.Label { text: qsTr("%1 days").arg(root.dayCount); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
        Details.DetailToolButton {
            objectName: "weatherPreviousDay"
            implicitWidth: 32; implicitHeight: 32
            display: PC.AbstractButton.IconOnly
            icon.name: "go-previous-symbolic"
            icon.width: 18; icon.height: 18
            text: qsTr("Previous forecast day")
            enabled: root.selectedDay > 0
            onClicked: root.selectDay(root.selectedDay - 1)
            PC.ToolTip { text: parent.text }
        }
        Details.DetailToolButton {
            objectName: "weatherNextDay"
            implicitWidth: 32; implicitHeight: 32
            display: PC.AbstractButton.IconOnly
            icon.name: "go-next-symbolic"
            icon.width: 18; icon.height: 18
            text: qsTr("Next forecast day")
            enabled: root.selectedDay < root.dayCount - 1
            onClicked: root.selectDay(root.selectedDay + 1)
            PC.ToolTip { text: parent.text }
        }
    }
    QQC.HorizontalHeaderView {
        id: dayHeader
        objectName: "weatherDayHeaders"
        x: 12; y: heading.y + heading.height + 8
        width: root.width - 24; height: metrics.height * 2 + 16
        model: root.futureDays
        textRole: "timestamp"
        clip: true; resizableColumns: false
        boundsBehavior: Flickable.StopAtBounds
        readonly property real cellWidth: Math.max(52, metrics.height * 3, width / Math.max(1, root.dayCount))
        columnWidthProvider: column => cellWidth
        rowHeightProvider: row => height
        onWidthChanged: forceLayout()
        delegate: PC.AbstractButton {
            id: day
            required property var model
            required property int column
            readonly property var date: model.timestamp
            readonly property bool validDate: date !== undefined && date !== null && !isNaN(date.getTime())
            implicitWidth: dayHeader.cellWidth
            implicitHeight: dayHeader.height
            topPadding: 6; bottomPadding: 6; leftPadding: 0; rightPadding: 0
            objectName: "weatherDay:" + column
            checkable: true
            checked: root.selectedDay === column
            text: validDate ? date.toLocaleString(Qt.locale(), "dddd, MMMM d") : qsTr("Forecast day %1").arg(column + 1)
            Accessible.name: text
            onClicked: root.selectDay(column)
            background: Details.DetailSurface {
                x: 2; y: 2; width: day.width - 4; height: day.height - 4
                radius: height / 2
                shadow.size: 0
                visible: day.checked || day.hovered || day.visualFocus
                emphasized: day.checked
                hovered: day.hovered && day.enabled
                pressed: day.down
                focused: day.visualFocus
            }
            contentItem: Column {
                spacing: 2
                PC.Label {
                    objectName: "weatherForecastDayLabel"
                    width: parent.width
                    text: day.validDate ? day.date.toLocaleString(Qt.locale(), "ddd") : "—"
                    horizontalAlignment: Text.AlignHCenter; textFormat: Text.PlainText
                    font.weight: day.checked ? Font.DemiBold : Font.Normal
                }
                PC.Label {
                    width: parent.width
                    text: day.validDate ? day.date.toLocaleString(Qt.locale(), "d") : ""
                    horizontalAlignment: Text.AlignHCenter; color: Kirigami.Theme.disabledTextColor
                    font: Kirigami.Theme.smallFont
                }
            }
        }
    }
    TableView {
        id: forecast
        objectName: "weatherForecastTable"
        x: 12; y: dayHeader.y + dayHeader.height + 8
        width: root.width - 24
        height: root.periodCount * root.periodHeight
        contentWidth: root.dayCount * width
        model: root.futureDays
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        columnWidthProvider: column => width
        rowHeightProvider: row => root.periodHeight
        onWidthChanged: {
            forceLayout();
            realign.restart();
        }
        onModelChanged: {
            forceLayout();
            realign.restart();
        }
        onDraggingChanged: if (dragging) daySlide.stop()
        onMovementEnded: {
            root.selectDay(Math.round(contentX / Math.max(1, width)));
            root.alignDay(true);
        }
        delegate: Item {
            id: cell
            required property var model
            required property int row
            implicitWidth: forecast.width; implicitHeight: root.periodHeight
            RowLayout {
                anchors.fill: parent; anchors.margins: 8; spacing: 12
                Kirigami.Icon {
                    Layout.preferredWidth: 40; Layout.preferredHeight: 40
                    source: cell.model.conditionIcon || Weather.Util.unknownWeatherIcon
                    fallback: Weather.Util.unknownWeatherIcon
                    PlasmaCore.ToolTipArea { anchors.fill: parent; mainText: cell.model.condition || qsTr("No forecast available") }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2
                    PC.Label {
                        Layout.fillWidth: true
                        text: root.hasNight ? (cell.row === 0 ? qsTr("Day") : qsTr("Night")) : qsTr("Daily forecast")
                        font.weight: Font.DemiBold
                    }
                    PC.Label {
                        Layout.fillWidth: true
                        text: cell.model.condition || qsTr("No forecast available")
                        textFormat: Text.PlainText; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                        color: Kirigami.Theme.disabledTextColor
                    }
                    PC.Label {
                        visible: root.futureDays?.hasProbability ?? false
                        text: qsTr("Precipitation: %1").arg(cell.model.conditionProbability !== undefined && cell.model.conditionProbability !== null && cell.model.conditionProbability !== "" && !isNaN(cell.model.conditionProbability)
                            ? qsTr("%1%").arg(cell.model.conditionProbability) : "—")
                        color: Kirigami.Theme.disabledTextColor; font: Kirigami.Theme.smallFont; textFormat: Text.PlainText
                    }
                }
                ColumnLayout {
                    spacing: 2
                    PC.Label {
                        objectName: "weatherForecastTemperature"
                        text: root.temperature(cell.model.highTemp)
                        visible: root.validTemperature(cell.model.highTemp) || !root.hasNight
                        font.weight: Font.DemiBold; textFormat: Text.PlainText
                    }
                    PC.Label {
                        objectName: "weatherForecastTemperature"
                        text: root.temperature(cell.model.lowTemp)
                        visible: root.validTemperature(cell.model.lowTemp) || !root.hasNight
                        color: root.hasNight ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                        font.weight: root.hasNight ? Font.DemiBold : Font.Normal
                        textFormat: Text.PlainText
                    }
                }
            }
        }
    }
    NumberAnimation {
        id: daySlide
        target: forecast; property: "contentX"
        duration: root.motionDuration; easing.type: Easing.OutCubic
    }
    function validTemperature(value) {
        return value !== null && value !== undefined && value !== "" && !isNaN(value);
    }
    function temperature(value) {
        return validTemperature(value) && metaData?.temperatureUnit
            ? Weather.Util.temperatureToDisplayString(displayTemperatureUnit, value, metaData.temperatureUnit, true) : "—";
    }
}
