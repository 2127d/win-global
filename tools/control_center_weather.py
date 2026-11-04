# SPDX-License-Identifier: MIT
"""Preserve KDE weather backends and unit conversion while replacing the popup."""
import re
import shutil


def style_weather(destination, source, plugin_dir, extract, replace):
    plugin = plugin_dir / 'org.kde.plasma.weather.so'
    if not plugin.exists():
        return []
    directory = destination / 'weather'
    extract(plugin, ':/qt/qml/plasma/applet/org/kde/plasma/weather/', directory)
    native_types = ('ForecastControl', 'LocationsControl', 'TemperatureUnitListModel',
                    'WindSpeedUnitListModel', 'PressureUnitListModel',
                    'VisibilityUnitListModel', 'Util', 'Warnings')
    for path in directory.glob('*.qml'):
        text = path.read_text()
        text = text.replace('import plasma.applet.org.kde.plasma.weather\n', '')
        text = text.replace('import QtQuick\n', 'import QtQuick\nimport plasma.applet.org.kde.plasma.weather as NativeWeather\n', 1)
        text = re.sub(r'\b(' + '|'.join(native_types) + r')\b', r'NativeWeather.\1', text)
        path.write_text(text)
    path = directory / 'SwitchPanel.qml'
    text = path.read_text()
    text = replace(text, 'import QtQuick\n', 'import QtQuick\nimport "../details" as Details\n')
    text = replace(text, '    id: root\n', '    id: root\n    property int selectedForecastDay: 0\n')
    text = replace(text, '        ForecastView {', '''        ForecastView {
            selectedDay: root.selectedForecastDay
            onDaySelected: index => root.selectedForecastDay = index''')
    text = replace(text, '    readonly property list<DetailsString> detailsModel: [', '''    readonly property list<DetailsString> detailsModel: [
        DetailsString {
            label: i18nc("@label", "Wind:")
            visible: root.lastObservation?.windSpeed !== undefined && root.lastObservation.windSpeed !== null
                && !isNaN(root.lastObservation.windSpeed) && !!root.metaData?.windSpeedUnit
            text: visible ? (root.lastObservation.windSpeed === 0 ? i18nc("Wind condition", "Calm")
                : NativeWeather.Util.valueToDisplayString(root.displaySpeedUnit, root.lastObservation.windSpeed, root.metaData.windSpeedUnit, 1)) : ""
        },
        DetailsString {
            label: i18nc("@label", "Wind Direction:")
            visible: !!root.lastObservation?.windDirection
            text: visible ? root.lastObservation.windDirection : ""
        },''')
    start = text.index('    PlasmaComponents.TabBar {')
    end = text.index('    Component {\n        id: forecastView', start)
    text = text[:start] + '''    readonly property real naturalHeight: (tabBar.visible ? tabBar.implicitHeight + spacing : 0) + Math.ceil(page.height) + 2
    spacing: 8
    Details.DetailTabBar {
        id: tabBar
        objectName: "weatherTabs"
        Layout.fillWidth: true
        implicitWidth: 0
        leftPadding: 0; rightPadding: 0; spacing: 6
        visible: root.pagesModel.length > 1
        Repeater {
            model: root.pagesModel
            delegate: Details.DetailTabButton {
                required property var modelData
                leftInset: 0; rightInset: 0
                text: modelData.title
                Accessible.name: text
            }
        }
    }
    onPagesModelChanged: Qt.callLater(() => { tabBar.currentIndex = Math.max(0, Math.min(tabBar.currentIndex, root.pagesModel.length - 1)); })
    PlasmaComponents.ScrollView {
        id: scroll
        objectName: "weatherPageScroll"
        Layout.fillWidth: true; Layout.fillHeight: true
        leftPadding: 0; rightPadding: 0
        PlasmaComponents.ScrollBar.horizontal.policy: PlasmaComponents.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true
            contentWidth: width; contentHeight: page.height
            boundsBehavior: Flickable.StopAtBounds
            WeatherTabStack {
                id: page
                objectName: "weatherPageStack"
                width: parent.width
                height: implicitHeight
                selectedIndex: tabBar.currentIndex
                sourceComponent: root.pagesModel[tabBar.currentIndex]?.view ?? null
                onCurrentItemChanged: scroll.contentItem.contentY = 0
            }
        }
    }

''' + text[end:]
    text = text.replace('            interactive: swipeView.contentItem.atXEnd\n', '')
    text = replace(text, '''        ColumnLayout {
            PlasmaExtras.PlaceholderMessage {''', '''        WeatherCard {
            implicitHeight: forecastMessage.implicitHeight + 48
            PlasmaExtras.PlaceholderMessage {
                id: forecastMessage
                anchors.fill: parent
                anchors.margins: 24''')
    # Zero degrees / zero humidity / zero visibility are valid observations.
    for field in ('dewpoint', 'pressure', 'humidity', 'windGust', 'visibility'):
        valid = f'!isNaN(root.lastObservation.{field})'
        if field == 'visibility':
            valid = f'(typeof root.lastObservation.visibility === "string" || {valid})'
        text = re.sub(r'!!root\.lastObservation\?\.' + field + r'\b',
                      f'(root.lastObservation?.{field} !== undefined && root.lastObservation.{field} !== null && {valid})', text)
    path.write_text(text)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '    id: root\n    readonly property bool sequoiaFooterActions: true\n    preloadFullRepresentation: true\n')
    start = text.index('    Plasmoid.title:')
    end = text.index('\n', start)
    text = text[:start] + '    Plasmoid.title: i18n("Weather")' + text[end:]
    text = replace(text, '    fullRepresentation: FullRepresentation {', '''    fullRepresentation: FullRepresentation {
        canConfigure: Plasmoid.internalAction("configure")?.enabled ?? false
        onConfigureRequested: Plasmoid.internalAction("configure").trigger()
        onSourceRequested: source => Qt.openUrlExternally(source)''')
    path.write_text(text)
    for path in (source / 'weather').glob('*.qml'):
        shutil.copy2(path, directory / path.name)
    return [plugin]
