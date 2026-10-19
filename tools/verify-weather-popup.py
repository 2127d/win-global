#!/usr/bin/env python3
"""Render all weather pages with deterministic models, without requesting weather."""
import os
from pathlib import Path
import sys
import tempfile
from preview_session import ensure_private_session

ensure_private_session()

ROOT = Path(__file__).resolve().parents[1]
dark = '--dark' in sys.argv
large = '--large' in sys.argv
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
from PySide6.QtCore import QObject, Property, Slot, QAbstractTableModel, QAbstractListModel, QModelIndex, Qt, QUrl, QPluginLoader, qInstallMessageHandler
from PySide6.QtGui import QPalette, QColor, QFont
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

class Translations(QObject):
    @Slot(str, result=str)
    def i18n(self, text): return text
    @Slot(str, str, result=str)
    def i18nc(self, context, text): return text
    @Slot(str, str, str, int, result=str)
    def i18ncp(self, context, one, many, count): return (one if count == 1 else many).replace('%1', str(count))

class Forecast(QAbstractTableModel):
    names = ['conditionIcon', 'conditionProbability', 'highTemp', 'lowTemp', 'condition', 'monthDay', 'weekDay']
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
    source = ROOT / '.staging/control-center-data/sequoia-eleven/control-center/weather'
    qml.write_text('''import QtQuick
import plasma.applet.org.kde.plasma.weather as Weather
import "''' + source.as_uri() + '''" as Custom
Rectangle {
    width: 520; height: 640
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
            credit: "Data from NOAA National Weather Service"})
        lastObservation: ({temperature: 24, currentConditions: "Mostly clear", conditionIcon: "weather-clear",
            dewpoint: 0, pressure: 1013, pressureTendency: "Steady", visibility: 20, humidity: 0, windGust: 15})
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
    tabs = find('weatherTabs')
    assert tabs.property('count') == 3
    output = ROOT / 'preview'; output.mkdir(exist_ok=True)
    suffix = ('dark' if dark else 'light') + ('-large' if large else '') + ('-rtl' if '--rtl' in sys.argv else '') + ('-daily' if '--day-only' in sys.argv else '')
    for index, page in enumerate(('forecast', 'details', 'notices')):
        tabs.setProperty('currentIndex', index); QTest.qWait(300)
        names = [i.objectName() for i in items(view.rootObject())]
        if index == 0:
            assert 'weatherDayCard' in names
            assert ('weatherNightCard' in names) == forecast.isNightPresent
            for row in range(forecast.rowCount()):
                table = find('weatherForecastTable' + str(row))
                assert table.property('rows') == forecast.rowCount() and table.property('columns') == 7
        if index == 1: assert names.count('weatherDetailCard') == 6, names
        if index == 2: assert names.count('weatherNoticeCard') == 2
        for card in items(view.rootObject()):
            if card.objectName().startswith('weather') and card.objectName().endswith('Card'):
                for child in card.childItems():
                    if child.isVisible() and child.width() > 0:
                        assert child.x() >= 12 and child.y() >= 12, (card.objectName(), child.x(), child.y())
                        assert child.x() + child.width() <= card.width() - 11, card.objectName()
                        assert child.y() + child.height() <= card.height() - 11, card.objectName()
        assert view.grabWindow().save(str(output / f'weather-{suffix}-{page}.png'))
    weather = find('sequoiaWeatherPage')
    tabs.setProperty('currentIndex', 0)
    weather.setProperty('futureDays', None)
    QTest.qWait(100)
    for state in ('timeoutState', 'connectingState', 'configurationState'):
        weather.setProperty('status', view.rootObject().property(state))
        QTest.qWait(100)
    view.setSource(QUrl())
    view.close()
    assert not errors, errors
# KDE's weather plugin can outlive Qt's font database during Python teardown.
# Like the native tray check, skip global teardown in this test-only process.
print('PASS: forecast periods, padded cards, zero readings, details/notices tabs and unavailable/loading states', flush=True)
os._exit(0)
