// SPDX-License-Identifier: GPL-2.0-or-later
// Sequoia presentation; KDE's location, forecast and unit models remain native.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.kholidays
import org.kde.kweather

Kirigami.Page {
    id: dashboard
    objectName: "sequoiaWeatherDashboard"
    padding: 0
    titleDelegate: null
    background: Item {}
    property int currentIndex: 0
    readonly property bool dynamic: settingsModel.forecastStyle === "Dynamic"
    readonly property bool sidebar: width >= 980
    readonly property var location: WeatherLocationListModel.locations[currentIndex] || null
    readonly property var days: location ? location.dayForecasts : []
    readonly property var current: location ? location.currentHourForecast : null
    readonly property bool hasWeather: current !== null && current !== undefined && days.length > 0
    readonly property var selected: days.length ? days[Math.max(0, Math.min(location.selectedDay, days.length-1))] : null
    readonly property color ink: dynamic ? "#f1f6fc" : Kirigami.Theme.textColor
    readonly property color muted: dynamic ? "#bdd0df" : Kirigami.Theme.disabledTextColor
    readonly property color accent: dynamic ? "#9bdbf0" : Kirigami.Theme.highlightColor
    readonly property real rangeMin: days.length ? Math.min(...days.map(d=>d.minTemp)) : 0
    readonly property real rangeMax: days.length ? Math.max(...days.map(d=>d.maxTemp)) : 1
    function switchPageIndex(index) { currentIndex = Math.max(0,Math.min(index,WeatherLocationListModel.count-1)); }
    function temp(value) { return Formatter.formatTemperatureRounded(value,settingsModel.temperatureUnits); }
    function selectDay(index) { if(location) location.selectedDay=index; }
    Connections {
        target: WeatherLocationListModel
        function onLocationsChanged() { dashboard.switchPageIndex(dashboard.currentIndex); }
    }
    onCurrentIndexChanged: { forecastScroll.contentItem.contentY = 0; forecastReveal.restart(); }
    NumberAnimation { id: forecastReveal; target: forecastScroll; property: "opacity"; from: 0.55; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }

    component Heading: Label { color: dashboard.ink; font.weight: Font.DemiBold; font.pixelSize: 16 }
    component Caption: Label { color: dashboard.muted; font.pixelSize: 12 }
    component Action: WeatherAction { atmospheric: dashboard.dynamic }
    component Surface: WeatherSurface { atmospheric: dashboard.dynamic }
    component Metric: Surface {
        id: metric
        property string title
        property string value
        property string detail
        property string symbol
        Layout.fillWidth: true
        implicitHeight: 132
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 18; spacing: 6
            RowLayout {
                Kirigami.Icon { source: metric.symbol; color: dashboard.muted; isMask: true; implicitWidth: 16; implicitHeight: 16 }
                Caption { text: metric.title; Layout.fillWidth: true; elide: Text.ElideRight }
            }
            Label { text: metric.value; color: dashboard.ink; font.pixelSize: 25; font.weight: Font.Medium; Layout.fillWidth: true; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 16 }
            Caption { text: metric.detail; Layout.fillWidth: true; elide: Text.ElideRight }
        }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            visible: dashboard.sidebar
            Layout.preferredWidth: 214; Layout.fillHeight: true
            color: dashboard.dynamic ? "#47121f31" : "transparent"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 10
                RowLayout {
                    Layout.topMargin: 12; Layout.bottomMargin: 24
                    Kirigami.Icon { source: "weather-few-clouds"; implicitWidth: 25; implicitHeight: 25 }
                    Heading { text: i18n("Weather"); font.pixelSize: 21 }
                }
                Caption { text: i18n("YOUR LOCATIONS"); font.letterSpacing: 1.2; font.pixelSize: 10; Layout.leftMargin: 10; Layout.bottomMargin: 2 }
                ListView {
                    id: cities
                    objectName: "weatherCities"
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: WeatherLocationListModel.locations
                    spacing: 8; clip: true
                    delegate: ItemDelegate {
                        id: city
                        required property var modelData
                        required property int index
                        width: cities.width; height: 88
                        onClicked: dashboard.switchPageIndex(index)
                        Accessible.name: modelData.name
                        background: Rectangle {
                            radius: 18
                            color: city.index === dashboard.currentIndex ? (dashboard.dynamic ? "#384e6588" : Qt.alpha(Kirigami.Theme.highlightColor,0.12)) : city.hovered ? Qt.alpha(dashboard.ink,0.06) : "transparent"
                            border.color: city.visualFocus ? dashboard.accent : "transparent"; border.width: 2
                            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                        }
                        contentItem: ColumnLayout {
                            spacing: 3
                            RowLayout {
                                Heading { text: city.modelData.name; font.pixelSize: 15; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: city.modelData.currentHourForecast ? dashboard.temp(city.modelData.currentHourForecast.temperature) : "—"; color: dashboard.ink; font.pixelSize: 22; font.weight: Font.Light }
                            }
                            Caption { text: city.modelData.currentTime; font.pixelSize: 11 }
                            Caption { text: city.modelData.currentHourForecast?.weatherDescription || i18n("Waiting for forecast"); Layout.fillWidth: true; elide: Text.ElideRight; font.pixelSize: 11 }
                        }
                    }
                    ScrollBar.vertical: WeatherScrollBar { tint: dashboard.ink }
                }
                Action { text: i18n("Add location"); symbol: "list-add"; Layout.fillWidth: true; onClicked: applicationWindow().openAddLocation() }
                Action { text: i18n("Manage locations"); symbol: "find-location"; Layout.fillWidth: true; onClicked: applicationWindow().openLocationsList() }
                Caption { text: i18n("SEQUOIA WEATHER"); font.pixelSize: 9; font.letterSpacing: 1.6; Layout.topMargin: 14; Layout.leftMargin: 10; Layout.bottomMargin: 8 }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            RowLayout {
                Layout.fillWidth: true; Layout.margins: dashboard.width < 560 ? 12 : 24; spacing: 6
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 3
                    Heading { text: dashboard.sidebar ? i18n("Forecast") : dashboard.location?.name || i18n("Weather"); font.pixelSize: dashboard.width < 560 ? 18 : 22; elide: Text.ElideRight; Layout.fillWidth: true }
                    Caption { visible: dashboard.width >= 600; text: dashboard.location?.currentDate || ""; Layout.fillWidth: true; elide: Text.ElideRight }
                }
                Action { visible: !dashboard.sidebar; text: i18n("Locations"); symbol: "find-location"; iconOnly: true; onClicked: applicationWindow().openLocationsList() }
                Rectangle {
                    implicitWidth: styleRow.implicitWidth + 6; implicitHeight: 42; radius: 14
                    color: Qt.alpha(dashboard.ink,0.05)
                    RowLayout {
                        id: styleRow; anchors.centerIn: parent; spacing: 2
                        Action { objectName: "weatherFlatStyle"; text: i18n("Flat"); selected: !dashboard.dynamic; onClicked: { settingsModel.forecastStyle="Flat";settingsModel.save(); } }
                        Action { objectName: "weatherDynamicStyle"; text: i18n("Dynamic"); selected: dashboard.dynamic; onClicked: { settingsModel.forecastStyle="Dynamic";settingsModel.save(); } }
                    }
                }
                Action { objectName: "weatherRefresh"; text: i18n("Refresh"); symbol: "view-refresh"; iconOnly: true; enabled: !!dashboard.location && !dashboard.location.loading; onClicked: dashboard.location.update() }
                Action { objectName: "weatherSettings"; text: i18n("Settings"); symbol: "settings-configure"; iconOnly: true; onClicked: applicationWindow().openSettings() }
            }
            ScrollView {
                id: forecastScroll
                objectName: "weatherScroll"
                Layout.fillWidth: true; Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: WeatherScrollBar {
                    tint: dashboard.ink
                    parent: forecastScroll
                    x: forecastScroll.width - width
                    y: 0
                    height: forecastScroll.height
                }
                ColumnLayout {
                    width: forecastScroll.availableWidth
                    spacing: 18
                    property real inset: dashboard.width < 560 ? 12 : 24
                    RowLayout {
                        visible: !!dashboard.location && (dashboard.location.loading || dashboard.location.errorMessage.length > 0)
                        Layout.leftMargin: parent.inset; Layout.rightMargin: parent.inset; Layout.fillWidth: true
                        BusyIndicator { visible: !!dashboard.location?.loading; running: visible; implicitWidth: 20; implicitHeight: 20 }
                        Caption { text: dashboard.location?.loading ? i18n("Updating forecast…") : (dashboard.days.length ? i18n("Could not refresh. Showing the last available forecast.") : i18n("Could not load the forecast. Check your connection and try again.")); wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    Surface {
                        id: hero
                        objectName: "weatherHero"
                        Layout.leftMargin: parent.inset; Layout.rightMargin: parent.inset; Layout.fillWidth: true
                        implicitHeight: 270
                        color: dashboard.dynamic ? "#3617253a" : flatSurface
                        ColumnLayout {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                            anchors.margins: 24
                            width: parent.width * 0.64 - 24; spacing: 3
                            Caption { text: i18n("CURRENT WEATHER"); font.pixelSize: 10; font.letterSpacing: 1.5 }
                            Heading { text: dashboard.location?.name || ""; font.pixelSize: hero.width < 420 ? 24 : 30; Layout.fillWidth: true; elide: Text.ElideRight }
                            RowLayout {
                                spacing: 0
                                Label {
                                    objectName: "weatherTemperature"
                                    text: dashboard.hasWeather ? Math.round(Formatter.convertTemp(dashboard.current.temperature,settingsModel.temperatureUnits)) : "—"
                                    color: dashboard.ink; font.pixelSize: hero.width < 420 ? 76 : 90; font.weight: Font.Light; font.letterSpacing: -4
                                }
                                Label { text: Formatter.formatTemperatureUnitDegrees(settingsModel.temperatureUnits); color: dashboard.ink; font.pixelSize: 29; font.weight: Font.Light; Layout.alignment: Qt.AlignTop; Layout.topMargin: 15 }
                            }
                            Heading { text: dashboard.current?.weatherDescription || i18n("Forecast unavailable"); font.pixelSize: 16; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Caption { text: dashboard.days.length ? i18n("High %1   ·   Low %2",dashboard.temp(dashboard.days[0].maxTemp),dashboard.temp(dashboard.days[0].minTemp)) : "" }
                        }
                        WeatherGlyph {
                            condition: dashboard.current?.weatherIcon || ""
                            visible: dashboard.hasWeather
                            anchors.right: parent.right; anchors.rightMargin: hero.width < 420 ? 6 : 30
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(190,hero.width * 0.34); height: width * 0.88
                        }
                        Caption {
                            anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.rightMargin: 20; anchors.bottomMargin: 48
                            visible: dashboard.hasWeather
                            text: dashboard.hasWeather ? i18n("Wind %1",Formatter.formatWindSpeed(dashboard.current.windSpeed,settingsModel.speedUnits)) : ""
                        }
                        Caption { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 20; text: dashboard.location?.lastUpdated ? i18n("Updated %1",dashboard.location.lastUpdated) : ""; font.pixelSize: 10 }
                    }
                    Surface {
                        Layout.leftMargin: parent.inset; Layout.rightMargin: parent.inset; Layout.fillWidth: true
                        visible: dashboard.hasWeather
                        implicitHeight: 202
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 18; spacing: 12
                            RowLayout {
                                Heading { text: i18n("Hourly outlook"); Layout.fillWidth: true }
                                Caption { text: dashboard.selected ? Qt.formatDate(dashboard.selected.date,"ddd, MMM d") : "" }
                            }
                            ListView {
                                id: hours
                                objectName: "weatherHours"
                                Layout.fillWidth: true; Layout.fillHeight: true
                                orientation: ListView.Horizontal; spacing: 6; clip: true
                                model: dashboard.location?.hourForecasts || []
                                delegate: Item {
                                    required property var modelData
                                    width: 84; height: hours.height - 12
                                    ColumnLayout {
                                        anchors.fill: parent; spacing: 7
                                        Caption { text: Formatter.formatHourlyCardDelegateTime(modelData.date,dashboard.location.timeZone); Layout.alignment: Qt.AlignHCenter }
                                        Kirigami.Icon { source: modelData.weatherIcon; implicitWidth: 29; implicitHeight: 29; Layout.alignment: Qt.AlignHCenter }
                                        Heading { text: dashboard.temp(modelData.temperature); Layout.alignment: Qt.AlignHCenter; font.pixelSize: 17 }
                                        Caption { text: Formatter.formatPrecipitation(modelData.precipitationAmount,settingsModel.precipitationUnits); color: dashboard.accent; Layout.alignment: Qt.AlignHCenter; font.pixelSize: 11 }
                                    }
                                    HoverHandler { id: hourHover }
                                    ToolTip.visible: hourHover.hovered
                                    ToolTip.text: modelData.weatherDescription + "\n" + i18n("Wind: %1", Formatter.formatWindSpeed(modelData.windSpeed,settingsModel.speedUnits))
                                }
                                ScrollBar.horizontal: WeatherScrollBar { policy: ScrollBar.AsNeeded; tint: dashboard.ink }
                            }
                        }
                    }
                    GridLayout {
                        Layout.leftMargin: parent.inset; Layout.rightMargin: parent.inset; Layout.fillWidth: true
                        visible: dashboard.hasWeather
                        columns: width >= 700 ? 2 : 1; columnSpacing: 18; rowSpacing: 18
                        Surface {
                            Layout.fillWidth: true; Layout.preferredWidth: 440; Layout.alignment: Qt.AlignTop
                            implicitHeight: dayColumn.implicitHeight + 36
                            ColumnLayout {
                                id: dayColumn
                                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 18
                                spacing: 8
                                RowLayout { Heading { text: i18n("Daily forecast"); Layout.fillWidth: true } Caption { text: i18np("%1 day","%1 days",dashboard.days.length) } }
                                Caption { text: i18n("Choose a day to explore its forecast"); Layout.bottomMargin: 8; Layout.fillWidth: true; wrapMode: Text.Wrap }
                                Repeater {
                                    model: dashboard.days
                                    delegate: ItemDelegate {
                                        id: day
                                        required property var modelData
                                        required property int index
                                        objectName: "weatherDay" + index
                                        Layout.fillWidth: true; implicitHeight: 46
                                        padding: 10
                                        Accessible.name: Qt.formatDate(modelData.date,Qt.DefaultLocaleLongDate) + ", " + modelData.weatherDescription + ", " + dashboard.temp(modelData.maxTemp)
                                        onClicked: dashboard.selectDay(index)
                                        background: Rectangle {
                                            radius: 13
                                            color: dashboard.location?.selectedDay === day.index ? Qt.alpha(dashboard.accent,0.13) : day.hovered ? Qt.alpha(dashboard.ink,0.05) : "transparent"
                                            border.width: day.visualFocus ? 2 : 0; border.color: dashboard.accent
                                            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                                        }
                                        contentItem: RowLayout {
                                            spacing: 12
                                            Label { text: day.index===0 ? i18n("Today") : Qt.formatDate(day.modelData.date,"ddd"); color: dashboard.ink; Layout.preferredWidth: 46; font.weight: day.index===dashboard.location?.selectedDay ? Font.DemiBold : Font.Normal }
                                            Kirigami.Icon { source: day.modelData.weatherIcon; implicitWidth: 24; implicitHeight: 24 }
                                            Caption { text: dashboard.temp(day.modelData.minTemp); Layout.preferredWidth: 34; horizontalAlignment: Text.AlignRight }
                                            Rectangle {
                                                Layout.fillWidth: true; Layout.minimumWidth: 30; implicitHeight: 5; radius: 3
                                                color: Qt.alpha(dashboard.ink,0.07)
                                                Rectangle {
                                                    x: (day.modelData.minTemp-dashboard.rangeMin)/Math.max(1,dashboard.rangeMax-dashboard.rangeMin) * parent.width
                                                    width: Math.max(6,(day.modelData.maxTemp-day.modelData.minTemp)/Math.max(1,dashboard.rangeMax-dashboard.rangeMin)*parent.width)
                                                    height: parent.height; radius: 3
                                                    gradient: Gradient { orientation: Gradient.Horizontal; GradientStop {position:0;color:dashboard.dynamic?"#7bc5df":"#56a5b8"} GradientStop {position:1;color:"#e6b97e"} }
                                                }
                                            }
                                            Heading { text: dashboard.temp(day.modelData.maxTemp); Layout.preferredWidth: 36; horizontalAlignment: Text.AlignRight; font.pixelSize: 13 }
                                        }
                                        ToolTip.visible: hovered
                                        ToolTip.text: modelData.weatherDescription
                                    }
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.preferredWidth: 340; Layout.alignment: Qt.AlignTop; spacing: 18
                            RowLayout {
                                Heading { text: i18n("The details"); Layout.fillWidth: true }
                                Caption { text: dashboard.selected ? Qt.formatDate(dashboard.selected.date,"ddd, MMM d") : "" }
                            }
                            GridLayout {
                                Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 12
                                Metric { title: i18n("Precipitation"); symbol: "raindrop"; value: dashboard.selected ? Formatter.formatPrecipitation(dashboard.selected.precipitation,settingsModel.precipitationUnits) : "—"; detail: i18n("Daily total") }
                                Metric { title: i18n("Humidity"); symbol: "weather-fog"; value: dashboard.selected ? Formatter.formatPercent(dashboard.selected.humidity.toFixed(0)) : "—"; detail: i18n("Relative humidity") }
                                Metric { title: i18n("UV index"); symbol: "weather-clear-symbolic"; value: dashboard.selected ? Formatter.formatDouble(dashboard.selected.uvIndex.toFixed(1)) : "—"; detail: dashboard.selected ? (dashboard.selected.uvIndex < 3 ? i18n("Low") : dashboard.selected.uvIndex < 6 ? i18n("Moderate") : dashboard.selected.uvIndex < 8 ? i18n("High") : dashboard.selected.uvIndex < 11 ? i18n("Very high") : i18n("Extreme")) : "" }
                                Metric { title: i18n("Pressure"); symbol: "speedometer"; value: dashboard.selected ? Formatter.formatPressure(dashboard.selected.pressure,settingsModel.pressureUnits) : "—"; detail: i18n("Atmospheric pressure") }
                            }
                            Surface {
                                Layout.fillWidth: true; implicitHeight: 150
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 18; spacing: 10
                                    RowLayout { Kirigami.Icon { source:"weather-clear-symbolic";color:dashboard.accent;isMask:true;implicitWidth:18;implicitHeight:18 } Heading {text:i18n("Sun & moon");Layout.fillWidth:true} }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        ColumnLayout { Layout.fillWidth:true; Caption {text:i18n("Sunrise")} Heading {text:dashboard.selected ? Formatter.formatSunriseTime(SunRiseSet.utcSunrise(dashboard.selected.date,dashboard.location.latitude,dashboard.location.longitude),dashboard.location.timeZone) : "—"} }
                                        ColumnLayout { Layout.fillWidth:true; Caption {text:i18n("Sunset")} Heading {text:dashboard.selected ? Formatter.formatSunriseTime(SunRiseSet.utcSunset(dashboard.selected.date,dashboard.location.latitude,dashboard.location.longitude),dashboard.location.timeZone) : "—"} }
                                    }
                                    Caption { text: dashboard.selected ? Lunar.phaseNameAtDate(dashboard.selected.date) : ""; Layout.fillWidth: true; elide: Text.ElideRight }
                                }
                            }
                        }
                    }
                    Caption {
                        Layout.leftMargin: parent.inset; Layout.rightMargin: parent.inset; Layout.bottomMargin: 24; Layout.fillWidth: true
                        text: i18n("Forecast times are local to %1.",dashboard.location?.name || "")
                        horizontalAlignment: Text.AlignHCenter; font.pixelSize: 11; wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
