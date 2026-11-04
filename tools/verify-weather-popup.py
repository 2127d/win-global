#!/usr/bin/env python3
"""Render all weather pages with deterministic models, without requesting weather."""
import os
import argparse
from pathlib import Path
import sys
import tempfile
from preview_session import ensure_private_session

ensure_private_session()

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
for option in ('dark','large','rtl','day-only'): parser.add_argument('--' + option, action='store_true')
parser.add_argument('--source', type=Path, default=ROOT / '.staging/control-center-data/sequoia-eleven/control-center/weather')
parser.add_argument('--output', type=Path, default=ROOT / 'preview')
args = parser.parse_args()
dark = '--dark' in sys.argv
large = '--large' in sys.argv
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
from PySide6.QtCore import QObject, Property, Slot, QDateTime, QDate, QTime, QMetaObject, QPointF, QAbstractTableModel, QAbstractListModel, QModelIndex, Qt, QUrl, QPluginLoader, qInstallMessageHandler
from PySide6.QtGui import QPalette, QColor, QFont
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest, QSignalSpy

class Translations(QObject):
    @Slot(str, result=str)
    def i18n(self, text): return text
    @Slot(str, str, result=str)
    def i18nc(self, context, text): return text
    @Slot(str, str, str, int, result=str)
    def i18ncp(self, context, one, many, count): return (one if count == 1 else many).replace('%1', str(count))

class Forecast(QAbstractTableModel):
    names = ['conditionIcon', 'conditionProbability', 'highTemp', 'lowTemp', 'condition', 'monthDay', 'weekDay', 'timestamp']
    @Property(int, constant=True)
    def daysNumber(self): return 7
    @Property(bool, constant=True)
    def isNightPresent(self): return '--day-only' not in sys.argv
    @Property(bool, constant=True)
    def hasProbability(self): return True
    @Property(str, constant=True)
    def firstDayIcon(self): return 'weather-clear'
    def rowCount(self, parent=QModelIndex()): return 0 if parent.isValid() else (2 if self.isNightPresent else 1)
    def columnCount(self, parent=QModelIndex()): return 0 if parent.isValid() else 7
    def roleNames(self): return {256 + i: name.encode() for i, name in enumerate(self.names)}
    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or role < 256: return None
        col, night = index.column(), index.row() == 1
        values = ['weather-clear-night' if night else ('weather-showers' if col == 0 else 'weather-clear'),
                  [65, 0, 12, 4, 0, 3, 17][col], float('nan') if night else 24 + col,
                  12 + col if night or not self.isNightPresent else float('nan'), 'Mostly clear', str(22 + col), 'Tue']
        return values[role - 256] if role - 256 < len(values) else None
    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if orientation == Qt.Horizontal:
            if role in (Qt.DisplayRole, 261): return str(22 + section)
            if role == 263: return QDateTime(QDate(2026, 11, 3 + section), QTime(12,0))
            if role == 262: return ['Tue','Wed','Thu','Fri','Sat','Sun','Mon'][section]
        return None

class Notices(QAbstractListModel):
    names = ['priority', 'timestamp', 'description', 'info']
    @Property(int, constant=True)
    def count(self): return 2
    @Property(int, constant=True)
    def maxPriorityCount(self): return 3
    def rowCount(self, parent=QModelIndex()): return 0 if parent.isValid() else 2
    def roleNames(self): return {256 + i: name.encode() for i, name in enumerate(self.names)}
    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or role < 256: return None
        rows = [[3, 'Tuesday · 4:30 PM', 'Thunderstorms possible this evening. Gusty winds and brief heavy rain may affect exposed areas. Follow local weather updates.', 'https://example.com/notice'],
                [1, 'Updated at 4:15 PM', 'Forecast information is updated throughout the day. Conditions at your location may differ from the reporting station. ' * 5, '']]
        return rows[index.row()][role - 256] if role - 256 < 4 else None

errors = []
def message(kind, context, text):
    print(text, file=sys.stderr)
    if any(x in text for x in ('ReferenceError', 'TypeError', 'Binding loop', 'Unable to assign', 'Cannot assign', 'is not a type', 'unavailable')): errors.append(text)
qInstallMessageHandler(message)
with tempfile.TemporaryDirectory(prefix='sequoia-weather-preview-') as temp:
    os.environ['XDG_CONFIG_HOME'] = temp
    variant = 'dark' if dark else 'light'
    Path(temp, 'kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{variant.title()}Solid.colors').read_text() + f'\n[Icons]\nTheme=SequoiaTahoe-{variant}\n')
    Path(temp, 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant}\n')
    Path(temp, 'plasma').symlink_to(ROOT / 'themes/plasma')
    Path(temp, 'icons').symlink_to(ROOT / 'themes/icons')
    os.environ.update(XDG_DATA_HOME=temp, XDG_STATE_HOME=temp+'/state', XDG_CACHE_HOME=temp+'/cache')
    app = QApplication([])
    plugin = QPluginLoader('/usr/lib64/qt6/plugins/plasma/applets/org.kde.plasma.weather.so')
    assert plugin.load(), plugin.errorString()
    palette = QPalette()
    for role in (QPalette.Window, QPalette.Base, QPalette.Button): palette.setColor(role, QColor('#252629' if dark else '#edf0f4'))
    for role in (QPalette.Text, QPalette.WindowText, QPalette.ButtonText): palette.setColor(role, QColor('#f2f3f5' if dark else '#252629'))
    palette.setColor(QPalette.Highlight, QColor('#8db3ff' if dark else '#2563db'))
    palette.setColor(QPalette.HighlightedText, QColor('#161a22' if dark else '#ffffff'))
    app.setPalette(palette)
    app.setFont(QFont('Adwaita Sans', 13 if large else 10))
    if '--rtl' in sys.argv: app.setLayoutDirection(Qt.RightToLeft)
    forecast, notices, translations = Forecast(), Notices(), Translations()
    view = QQuickView()
    view.engine().addImportPath('qrc:/qt/qml')
    view.rootContext().setContextObject(translations)
    view.rootContext().setContextProperty('testForecast', forecast)
    view.rootContext().setContextProperty('testNotices', notices)
    qml = Path(temp, 'Preview.qml')
    source = args.source.resolve()
    qml.write_text('''import QtQuick
import plasma.applet.org.kde.plasma.weather as Weather
import "''' + source.as_uri() + '''" as Custom
Rectangle {
    width: weather.sequoiaPreferredWidth + 24; height: Math.min(720, weather.sequoiaPreferredHeight + 24)
    readonly property int timeoutState: Weather.ForecastControl.Timeout
    readonly property int connectingState: Weather.ForecastControl.Connecting
    readonly property int configurationState: Weather.ForecastControl.NeedsConfiguration
    color: "''' + ('#252629' if dark else '#edf0f4') + '''"
    Custom.FullRepresentation {
        id: weather
        anchors.fill: parent; anchors.margins: 12
        status: Weather.ForecastControl.Normal
        station: ({place: "Bolinder Field–Tooele Valley Airport"})
        futureDays: testForecast
        warnings: testNotices
        displayTemperatureUnit: Weather.TemperatureUnitListModel.unitIdForListIndex(0)
        displaySpeedUnit: Weather.WindSpeedUnitListModel.unitIdForListIndex(0)
        displayPressureUnit: Weather.PressureUnitListModel.unitIdForListIndex(0)
        displayVisibilityUnit: Weather.VisibilityUnitListModel.unitIdForListIndex(0)
        metaData: ({temperatureUnit: displayTemperatureUnit, pressureUnit: displayPressureUnit,
            windSpeedUnit: displaySpeedUnit, visibilityUnit: displayVisibilityUnit, humidityUnit: 1,
            credit: "Data from NOAA National Weather Service", creditURL: "https://example.com/forecast"})
        lastObservation: ({temperature: 24, currentConditions: "Mostly clear", conditionIcon: "weather-clear",
            windSpeed: 0, windDirection: "North", dewpoint: 0, pressure: 1013, pressureTendency: "Steady", visibility: 20, humidity: 0, windGust: 15})
    }
}''')
    view.setSource(QUrl.fromLocalFile(str(qml)))
    assert view.status() == QQuickView.Ready, view.errors()
    view.show(); QTest.qWait(1000)
    def items(item):
        result = [item]
        for child in item.childItems(): result.extend(items(child))
        return result
    def find(name): return next(i for i in items(view.rootObject()) if i.objectName() == name)
    weather = find('sequoiaWeatherPage')
    settings = QSignalSpy(weather.configureRequested)
    source_link = QSignalSpy(weather.sourceRequested)
    QMetaObject.invokeMethod(find('weatherSettings'), 'clicked')
    QMetaObject.invokeMethod(find('weatherSource'), 'clicked')
    assert settings.count() == 1 and source_link.count() == 1
    assert source_link.at(0)[0].toString() == 'https://example.com/forecast'
    tabs = find('weatherTabs')
    assert tabs.property('count') == 3
    output = args.output; output.mkdir(parents=True, exist_ok=True)
    suffix = ('dark' if dark else 'light') + ('-large' if large else '') + ('-rtl' if '--rtl' in sys.argv else '') + ('-daily' if '--day-only' in sys.argv else '')
    for index, page in enumerate(('forecast', 'details', 'notices')):
        tabs.setProperty('currentIndex', index); QTest.qWait(300)
        names = [i.objectName() for i in items(view.rootObject())]
        if index == 0:
            assert 'weatherForecastCard' in names
            table = find('weatherForecastTable')
            assert table.property('rows') == forecast.rowCount() and table.property('columns') == 7
            labels = [i.property('text') for i in items(view.rootObject()) if i.objectName() == 'weatherForecastDayLabel']
            assert labels and all(label and label != '—' for label in labels), labels
            for label in items(table):
                if label.objectName() == 'weatherForecastTemperature' and label.isVisible():
                    assert label.height() >= label.implicitHeight() - 1, (label.height(), label.implicitHeight())
                    bottom = label.mapToItem(table, QPointF(0, label.height())).y()
                    assert bottom <= table.height() + 1, (bottom, table.height())
            card = find('weatherForecastCard')
            card.setProperty('motionDuration', 240)
            next_day, previous_day = find('weatherNextDay'), find('weatherPreviousDay')
            assert not previous_day.property('enabled')
            QMetaObject.invokeMethod(next_day, 'clicked')
            QTest.qWait(65)
            assert card.property('selectedDay') == 1
            assert 0 < table.property('contentX') < table.width(), table.property('contentX')
            QTest.qWait(260)
            assert abs(table.property('contentX') - table.width()) < 1
            QMetaObject.invokeMethod(previous_day, 'clicked')
            QTest.qWait(65)
            assert 0 < table.property('contentX') < table.width()
            QTest.qWait(260)
            assert abs(table.property('contentX')) < 1
            QMetaObject.invokeMethod(find('weatherDay:4'), 'clicked')
            QTest.qWait(50)
            QMetaObject.invokeMethod(find('weatherDay:1'), 'clicked')
            QTest.qWait(300)
            assert card.property('selectedDay') == 1
            assert abs(table.property('contentX') - table.width()) < 1
            card.setProperty('motionDuration', 0)
            card.setProperty('selectedDay', 6)
            QTest.qWait(30)
            assert not next_day.property('enabled')
            assert abs(table.property('contentX') - 6 * table.width()) < 1
            card.setProperty('selectedDay', 0)
            QTest.qWait(30)
            for button in (next_day, previous_day):
                background = button.property('background')
                assert abs(background.width() - background.height()) < 1
            card.forceActiveFocus()
            QTest.keyClick(view, Qt.Key_Right); QTest.qWait(30)
            assert card.property('selectedDay') == 1
            QTest.keyClick(view, Qt.Key_Left); QTest.qWait(30)
            assert card.property('selectedDay') == 0
            QTest.mouseMove(view, next_day.mapToScene(QPointF(next_day.width()/2, next_day.height()/2)).toPoint())
            QTest.qWait(100)
            assert next_day.property('hovered')
            assert next_day.property('background').isVisible()
        if index == 1: assert names.count('weatherDetailRow') == 8, names
        if index == 2: assert names.count('weatherNoticeCard') == 2
        rail = find('sequoiaWeatherPage').width()
        for card in items(view.rootObject()):
            if card.objectName() in ('weatherCurrentCard','weatherForecastCard','weatherDetailsCard','weatherNoticeCard'):
                assert abs(card.width() - rail) < 1, (card.objectName(), card.width(), rail)
        assert find('weatherActionIsland').width() < rail
        if weather.property('sequoiaPreferredHeight') < 696:
            assert find('weatherPageScroll').height() >= find('weatherPageStack').height(), page
        assert view.grabWindow().save(str(output / f'weather-{suffix}-{page}.png'))
        if index == 0:
            QMetaObject.invokeMethod(find('weatherDay:3'), 'clicked')
            QTest.qWait(300)
    assert find('weatherCurrentCard').height() < (190 if large else 160)
    weather = find('sequoiaWeatherPage')
    tabs.setProperty('currentIndex', 0)
    QTest.qWait(300)
    assert find('weatherForecastCard').property('selectedDay') == 3
    table = find('weatherForecastTable')
    assert abs(table.property('contentX') - 3 * table.width()) < 1
    stack = find('weatherPageStack')
    stack.setProperty('motionDuration', 240)
    for destination, direction in ((1, 1), (2, 1), (1, -1), (0, -1)):
        outgoing = stack.property('currentItem')
        tabs.setProperty('currentIndex', destination)
        QTest.qWait(65)
        incoming = stack.property('currentItem')
        assert stack.property('busy')
        assert 0 < direction * incoming.x() < stack.width(), incoming.x()
        assert -stack.width() < direction * outgoing.x() < 0, outgoing.x()
        QTest.qWait(260)
        assert not stack.property('busy') and abs(incoming.x()) < 1
    tabs.setProperty('currentIndex', 2); QTest.qWait(45)
    tabs.setProperty('currentIndex', 0); QTest.qWait(300)
    assert not stack.property('busy')
    assert stack.property('currentItem').objectName() == 'weatherForecastCard'
    assert find('weatherForecastCard').property('selectedDay') == 3
    stack.setProperty('motionDuration', 0)
    tabs.setProperty('currentIndex', 1); QTest.qWait(30)
    assert not stack.property('busy') and abs(stack.property('currentItem').x()) < 1
    tabs.setProperty('currentIndex', 0); QTest.qWait(30)
    weather.setProperty('status', view.rootObject().property('connectingState'))
    QTest.qWait(100)
    assert weather.property('ready') and find('weatherUpdatingLabel').isVisible()
    assert find('weatherCurrentCard').isVisible()
    weather.setProperty('lastObservation', None)
    weather.setProperty('futureDays', None)
    QTest.qWait(100)
    for state in ('timeoutState', 'connectingState', 'configurationState'):
        weather.setProperty('status', view.rootObject().property(state))
        QTest.qWait(100)
        assert not weather.property('ready') and find('weatherStateCard').isVisible()
        assert view.grabWindow().save(str(output / f'weather-{suffix}-{state}.png'))
    view.setSource(QUrl())
    view.close()
    assert not errors, errors
# KDE's weather plugin can outlive Qt's font database during Python teardown.
# Like the native tray check, skip global teardown in this test-only process.
print('PASS: dated forecast periods, aligned cards, content-fitting footer, zero readings, details/notices tabs and unavailable/loading states', flush=True)
os._exit(0)
