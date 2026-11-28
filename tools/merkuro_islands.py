"""Adapt the installed Merkuro Calendar's QML, preserving its package notices."""
import hashlib
import os
import re
from pathlib import Path
import shlex
import subprocess


def build(root, destination, fingerprints, replace, background):
    binary = Path('/usr/bin/merkuro-calendar')
    if not binary.exists():
        return
    helper = root / '.staging/export-merkuro-qml.so'
    source = root / 'tools/export-merkuro-qml.cpp'
    helper.parent.mkdir(exist_ok=True)
    if not helper.exists() or helper.stat().st_mtime < source.stat().st_mtime:
        flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core'], text=True))
        subprocess.run(['c++', '-shared', '-fPIC', str(source), '-o', str(helper), *flags], check=True)
    module = destination / 'org/kde/merkuro/calendar'
    # LD_PRELOAD splits on spaces and colons and has no quoting syntax. An
    # inherited descriptor also works when the extracted checkout contains either.
    with helper.open('rb') as preload:
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen',
                   LD_PRELOAD=f'/proc/self/fd/{preload.fileno()}',
                   SEQUOIA_EXPORT_MERKURO=str(module))
        subprocess.run([str(binary)], env=env, pass_fds=(preload.fileno(),), check=True, timeout=15)
    for path in module.rglob('*'):
        if path.is_file():
            path.chmod(0o644)
    fingerprints[str(binary)] = hashlib.sha256(binary.read_bytes()).hexdigest()
    bridge_source = root / 'themes/application-style/quick/merkuro-qml.cpp'
    from PySide6.QtCore import QLibraryInfo
    package = root / 'apps/merkuro'
    moc = Path(QLibraryInfo.path(QLibraryInfo.LibraryExecutablesPath)) / 'moc'
    weather_moc = destination / 'moc_scheduleweather.cpp'
    subprocess.run([str(moc), str(package / 'scheduleweather.h'), '-o', str(weather_moc)], check=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Qml', 'Qt6Network', 'Qt6QuickWidgets'], text=True))
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(bridge_source), '-o',
                    str(destination / 'merkuro-qml.so'), str(package / 'scheduleweather.cpp'), str(package / 'accountdialogs.cpp'), str(weather_moc),
                    '-I' + str(package), '-I/usr/include/KF6/KConfigCore', '-I/usr/include/KF6/KConfig',
                    *flags, '-lKF6ConfigCore', '-ldl'], check=True)
    weather_moc.unlink()
    qmldir = module / 'qmldir'
    qmldir.write_text(''.join(line for line in qmldir.read_text().splitlines(True) if not line.startswith('prefer ')))

    def patch(relative, transform):
        path = module / 'qml' / relative
        path.write_text(transform(path.read_text()))

    from merkuro_settings import build as build_settings
    build_settings(module, fingerprints, replace)

    # The app-specific layout lives with the other system app makeovers.
    package = root / 'apps/merkuro'
    import shutil
    for source in (package / 'qml').glob('*.qml'):
        shutil.copy2(source, module / 'qml' / source.name)
        with qmldir.open('a') as stream:
            stream.write(f'{source.stem} 1.0 qml/{source.name}\n')

    def remove_title(t):
        start = t.index('    titleDelegate:')
        brace = t.index('{', start)
        depth = 0
        for match in re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/|[{}]', t[brace:], re.S):
            if match.group() == '{': depth += 1
            elif match.group() == '}':
                depth -= 1
                if depth == 0:
                    return t[:start] + '    titleDelegate: null' + t[brace + match.end():]
        raise RuntimeError('Unbalanced Merkuro page title delegate')

    island_import = 'import org.sequoia.components as S\n'
    for name in ('MonthView', 'HourlyView', 'ScheduleView', 'TodoView'):
        def page(t):
            if 'import QtQuick.Layouts' not in t:
                t = t.replace('import QtQuick\n', 'import QtQuick\nimport QtQuick.Layouts\n')
            t = t.replace('import QtQuick\n', 'import QtQuick\n' + island_import)
            t = replace(t, '    id: root', '    id: root\n    readonly property bool sequoiaMainPage: true\n    header: Calendar.SequoiaToolbar { page: root }')
            t = remove_title(t)
            t = replace(t, '    padding: 0', '    padding: 16\n    contentItem.clip: true')
            return background(t, '    background: Item {}')
        patch(f'MainViews/{name}.qml', page)

    def wrap_native(t, extra, before=''):
        # Preserve both upstream rendering modes and every native drag/drop handler.
        index = t.index('    Loader {\n        id: swipeableViewLoader')
        native = t[index:t.rfind('}')]
        return t[:index] + before + """
    RowLayout {
        anchors.fill: parent
        spacing: 16
        S.Panel {
            objectName: "sequoiaMerkuroCalendar"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 280
            followMaterial: false; tint: 1
            Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
            Item {
                anchors.fill: parent; anchors.margins: 12; clip: true
""" + native + """
            }
        }
""" + extra + '\n    }\n}\n'

    def month(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport QtCore\n')
        t = wrap_native(t, """
        GridLayout {
            objectName: "sequoiaMonthCalendarGrid"
            visible: root.sequoiaExtended
            columns: 2; rowSpacing: 16; columnSpacing: 16
            Layout.fillWidth: true; Layout.fillHeight: true
            Repeater {
                model: root.sequoiaExtended ? root.sequoiaVisibleMonths : 0
                delegate: S.Panel {
                    id: monthCard
                    required property int index
                    readonly property date month: new Date(root.sequoiaMonthStart.getFullYear(), root.sequoiaMonthStart.getMonth() + index, 1)
                    objectName: index === 0 ? "sequoiaMerkuroCalendar" : "sequoiaMonthCalendar" + index
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.minimumWidth: 300; Layout.minimumHeight: 250
                    Layout.preferredWidth: 1; Layout.preferredHeight: 1
                    followMaterial: false; tint: 1
                    Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
                    color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 8
                        Kirigami.Heading {
                            Layout.fillWidth: true; font.pixelSize: 17; font.weight: Font.DemiBold
                            text: Qt.formatDate(monthCard.month, "MMMM yyyy")
                        }
                        Item {
                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                            DayGridView {
                                objectName: "sequoiaExtendedMonthGrid" + monthCard.index
                                anchors.fill: parent
                                firstDayOfMonth: monthCard.month
                                startDate: Calendar.DateUtils.getFirstDayOfWeek(firstDayOfMonth)
                                dragDropEnabled: root.dragDropEnabled
                                openOccurrence: root.openOccurrence
                            }
                        }
                    }
                }
            }
        }
        Calendar.SequoiaAgenda {
            visible: root.width >= 820
            Layout.preferredWidth: Math.min(320, root.width * .29)
            Layout.maximumWidth: 320; Layout.fillWidth: false
            Layout.fillHeight: true
        }
""", """
    Settings {
        id: monthPreferences
        location: StandardPaths.writableLocation(StandardPaths.GenericConfigLocation) + "/sequoia-merkuro.ini"
        category: "Month"
        property bool extended: false
        property int viewCount: 2
    }
    readonly property bool sequoiaExtendedRequested: monthPreferences.extended
    readonly property int sequoiaRequestedMonths: Math.max(2, Math.min(4, monthPreferences.viewCount))
    readonly property int sequoiaVisibleMonths: !sequoiaExtendedRequested || width < 1100 ? 1
        : Math.min(sequoiaRequestedMonths, height >= 650 ? 4 : 2)
    readonly property bool sequoiaExtended: sequoiaVisibleMonths > 1
    property date sequoiaMonthStart: new Date(new Date().getFullYear(), new Date().getMonth(), 1)
    Component.onCompleted: sequoiaMonthStart = Calendar.DateTimeState.firstDayOfMonth
    function sequoiaChooseMonths(count) {
        count = Math.max(1, Math.min(4, Math.round(count)));
        if (count > 1) monthPreferences.viewCount = count;
        monthPreferences.extended = count > 1;
        monthPreferences.setValue("viewCount", monthPreferences.viewCount);
        monthPreferences.setValue("extended", monthPreferences.extended);
        monthPreferences.sync();
    }
    function sequoiaMoveMonth(offset) {
        sequoiaMonthStart = new Date(sequoiaMonthStart.getFullYear(), sequoiaMonthStart.getMonth() + offset, 1);
        Calendar.DateTimeState.selectedDate = sequoiaMonthStart;
    }
    Connections {
        target: Calendar.DateTimeState
        function onSelectedDateChanged() {
            const selected = Calendar.DateTimeState.selectedDate;
            const end = new Date(root.sequoiaMonthStart.getFullYear(), root.sequoiaMonthStart.getMonth() + root.sequoiaVisibleMonths, 1);
            if (!root.sequoiaExtended || selected < root.sequoiaMonthStart || selected >= end)
                root.sequoiaMonthStart = new Date(selected.getFullYear(), selected.getMonth(), 1);
        }
    }
""")
        t = replace(t, '            objectName: "sequoiaMerkuroCalendar"', '            objectName: "sequoiaMerkuroCalendar"\n            visible: !root.sequoiaExtended')
        t = t.replace('active: Calendar.Config.monthGridMode ===', 'active: !root.sequoiaExtended && Calendar.Config.monthGridMode ===')
        t = replace(t, 'onTriggered: Calendar.DateTimeState.selectPreviousMonth()', 'onTriggered: root.sequoiaMoveMonth(-1)')
        t = replace(t, 'onTriggered: Calendar.DateTimeState.selectNextMonth()', 'onTriggered: root.sequoiaMoveMonth(1)')
        t = replace(t, 'onTriggered: Calendar.DateTimeState.resetTime();', 'onTriggered: { Calendar.DateTimeState.resetTime(); root.sequoiaMonthStart = Calendar.DateTimeState.firstDayOfMonth; }')
        return t
    patch('MainViews/MonthView.qml', month)
    def hourly(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport QtCore\n')
        t = wrap_native(t, """
        S.Panel {
            objectName: "sequoiaNextWeekPanel"
            visible: root.sequoiaExtended
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
            Layout.minimumWidth: 480
            followMaterial: false; tint: 1
            Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
            Loader {
                id: nextWeekLoader
                anchors.fill: parent; anchors.margins: 12
                active: root.sequoiaExtended
                sourceComponent: BasicInternalHourlyView {
                    objectName: "sequoiaNextWeek"
                    anchors.fill: parent
                    startDate: Calendar.Utils.addDaysToDate(root.sequoiaWeekStart, 7)
                    daysToShow: 7
                    openOccurrence: root.openOccurrence
                    dragDropEnabled: root.dragDropEnabled
                }
            }
        }
""", """
    property bool sequoiaToday: false
    Settings {
        id: weekPreferences
        location: StandardPaths.writableLocation(StandardPaths.GenericConfigLocation) + "/sequoia-merkuro.ini"
        category: "Week"
        property bool extended: false
    }
    readonly property bool sequoiaExtendedRequested: weekPreferences.extended
    readonly property bool sequoiaExtended: !sequoiaToday && daysToShow === 7 && sequoiaExtendedRequested && width >= 1200
    readonly property date sequoiaWeekStart: swipeableViewLoader.active && swipeableViewLoader.item?.currentItem
        ? swipeableViewLoader.item.currentItem.startDate : Calendar.DateTimeState.firstDayOfWeek
    Calendar.SequoiaWeekScrollSync {
        id: weekScrollSync
        active: !root.sequoiaToday && root.daysToShow === 7
        enabled: root.sequoiaExtended
        firstView: swipeableViewLoader.active ? swipeableViewLoader.item?.currentItem?.item ?? null : basicViewLoader.item
        secondView: nextWeekLoader.item
    }
    function sequoiaToggleWeek() {
        weekPreferences.extended = !weekPreferences.extended;
        weekPreferences.setValue("extended", weekPreferences.extended);
        weekPreferences.sync();
    }
""")
        t = replace(t, 'onTriggered: Calendar.DateTimeState.resetTime();', 'onTriggered: { Calendar.DateTimeState.resetTime(); if (!root.sequoiaToday && root.daysToShow === 7) weekScrollSync.goToCurrentTime(); }')
        t = replace(t, '            objectName: "sequoiaMerkuroCalendar"', '            objectName: "sequoiaMerkuroCalendar"\n            Layout.preferredWidth: 1')
        t = replace(t, 'Layout.minimumWidth: 280', 'Layout.minimumWidth: root.sequoiaExtended ? 480 : 280')
        t = t.replace('active: Calendar.Config.hourlyViewMode ===', 'active: !root.sequoiaToday && Calendar.Config.hourlyViewMode ===')
        # The regular Week/Day pages retain their native renderer. Today uses
        # the same one-day timeline inside the five-card dashboard.
        t = replace(t, '    RowLayout {\n        anchors.fill: parent', '    RowLayout {\n        visible: !root.sequoiaToday\n        anchors.fill: parent')
        return t[:t.rfind('}')] + """
    Loader {
        anchors.fill: parent
        active: root.sequoiaToday
        sourceComponent: Calendar.SequoiaToday { page: root }
    }
}
"""
    patch('MainViews/HourlyView.qml', hourly)
    def schedule(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport QtCore\n')
        t = wrap_native(t, '''
        Repeater {
            model: root.sequoiaVisibleMonths - 1
            delegate: S.Panel {
                id: extraMonth
                required property int index
                readonly property date month: new Date(root.sequoiaScheduleMonth.getFullYear(), root.sequoiaScheduleMonth.getMonth() + index + 1, 1)
                objectName: index === 0 ? "sequoiaNextScheduleMonth" : "sequoiaExtraScheduleMonth" + (index + 1)
                radius: S.Tokens.panelRadius + 6
                Layout.fillWidth: true; Layout.fillHeight: true
                Layout.preferredWidth: 1; Layout.minimumWidth: 280; Layout.minimumHeight: 220
                followMaterial: false; tint: 1
                Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
                color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 12
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        text: Qt.formatDate(extraMonth.month, "MMMM yyyy")
                        font.pixelSize: 17; font.weight: Font.DemiBold
                    }
                    Loader {
                        objectName: extraMonth.index === 0 ? "sequoiaNextScheduleLoader" : "sequoiaExtraScheduleLoader" + (extraMonth.index + 1)
                        Layout.fillWidth: true; Layout.fillHeight: true
                        active: root.sequoiaExtended
                        sourceComponent: BasicMonthListView {
                            anchors.fill: parent
                            startDate: extraMonth.month
                            openOccurrence: root.openOccurrence
                            dragDropEnabled: root.dragDropEnabled
                            isCurrentItem: root.sequoiaExtended
                            onStartDateChanged: {
                                savedYScrollPos = 0;
                                Qt.callLater(moveToSelected);
                            }
                        }
                    }
                }
            }
        }
        Calendar.SequoiaScheduleWeather {
            month: root.sequoiaScheduleMonth
            monthCount: root.sequoiaVisibleMonths
            Layout.preferredWidth: root.width >= 820 ? Math.min(360, root.width * .34) : -1
            Layout.fillWidth: root.width < 820
            Layout.fillHeight: true
            Layout.minimumHeight: 180
        }
''', '''
    Settings {
        id: schedulePreferences
        location: StandardPaths.writableLocation(StandardPaths.GenericConfigLocation) + "/sequoia-merkuro.ini"
        category: "Schedule"
        property bool extended: false
        property int viewCount: 2
    }
    readonly property bool sequoiaExtendedRequested: schedulePreferences.extended
    readonly property int sequoiaRequestedMonths: Math.max(2, Math.min(4, schedulePreferences.viewCount))
    readonly property bool sequoiaCanExtend: width >= 1100
    readonly property int sequoiaVisibleMonths: !sequoiaExtendedRequested || !sequoiaCanExtend ? 1
        : Math.min(sequoiaRequestedMonths, 2 + Math.floor((width - 1100) / 340))
    readonly property bool sequoiaExtended: sequoiaVisibleMonths > 1
    function sequoiaChooseMonths(count) {
        count = Math.max(1, Math.min(4, Math.round(count)));
        if (count > 1) schedulePreferences.viewCount = count;
        schedulePreferences.extended = count > 1;
        // Flush explicitly so even an immediate restart restores this choice.
        schedulePreferences.setValue("viewCount", schedulePreferences.viewCount);
        schedulePreferences.setValue("extended", schedulePreferences.extended);
        schedulePreferences.sync();
    }
    readonly property date sequoiaScheduleMonth: {
        const date = swipeableViewLoader.active && swipeableViewLoader.item?.currentItem
            ? swipeableViewLoader.item.currentItem.firstDayOfMonth : Calendar.DateTimeState.currentDate;
        return new Date(date.getFullYear(), date.getMonth(), 1);
    }
''')
        t = replace(t, '    RowLayout {', '    GridLayout {\n        columns: root.width >= 820 ? (root.sequoiaVisibleMonths + 1) : 1')
        t = replace(t, '        spacing: 16', '        rowSpacing: 16; columnSpacing: 16')
        t = replace(t, 'startDate: Calendar.DateTimeState.currentDate', 'startDate: root.sequoiaScheduleMonth')
        t = replace(t, '        id: basicViewLoader',
                    '        id: basicViewLoader\n        objectName: "sequoiaPrimaryScheduleLoader"')
        t = replace(t, '            objectName: "sequoiaMerkuroCalendar"',
                    '            objectName: "sequoiaMerkuroCalendar"\n            radius: S.Tokens.panelRadius + 6\n            Layout.minimumHeight: 220\n            Layout.preferredWidth: 1')
        t = replace(t, '            Item {\n                anchors.fill: parent; anchors.margins: 12; clip: true', '''            Kirigami.Heading {
                id: sequoiaCurrentScheduleHeading
                visible: root.sequoiaExtended
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: 12
                text: Qt.formatDate(root.sequoiaScheduleMonth, "MMMM yyyy")
                font.pixelSize: 17; font.weight: Font.DemiBold
            }
            Item {
                anchors.fill: parent; anchors.margins: 12
                anchors.topMargin: root.sequoiaExtended ? sequoiaCurrentScheduleHeading.height + 24 : 12
                clip: true''')
        return t
    patch('MainViews/ScheduleView.qml', schedule)
    def schedule_days(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport org.sequoia.components as S\n')
        start = t.index('    function moveToSelected() {')
        end = t.index('\n    ListView {', start)
        t = t[:start] + """    function moveToSelected() { scheduleScrollMemory.requestRestore(); }
    property Calendar.SequoiaListScrollMemory sequoiaScrollMemory: Calendar.SequoiaListScrollMemory {
        id: scheduleScrollMemory
        objectName: "sequoiaScheduleScrollMemory"
        view: scheduleListView
        positionKey: "month/" + Qt.formatDate(scrollView.startDate, "yyyy-MM")
        active: scrollView.isCurrentItem && scrollView.visible
        defaultIndex: function() {
            const today = Calendar.DateTimeState.currentDate;
            return today.getMonth() === scrollView.startDate.getMonth() && today.getFullYear() === scrollView.startDate.getFullYear()
                ? today.getDate() - 1 : 0;
        }
    }
""" + t[end:]
        t = replace(t, '    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff', '''    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
    QQC2.ScrollBar.vertical: Calendar.SequoiaGlassScrollBar {
        parent: scrollView
        z: 20
        x: scrollView.mirrored ? 0 : scrollView.width - width
        y: scrollView.topPadding
        height: scrollView.availableHeight
    }''')
        t = replace(t, '            id: backgroundRectangle',
                    '            id: backgroundRectangle\n            radius: S.Tokens.panelRadius')
        t = replace(t, '                    id: dayGrid',
                    '                    id: dayGrid\n                    Layout.fillWidth: true\n                    Layout.minimumWidth: 0')
        t = replace(t, '                        id: cardsColumn',
                    '                        id: cardsColumn\n                        Layout.minimumWidth: 0')
        t = replace(t, '        id: scheduleListView',
                    '''        id: scheduleListView
        objectName: "sequoiaNativeScheduleDays"
        S.Theme { id: scheduleFadeTheme }
        Rectangle {
            parent: scheduleListView
            objectName: "sequoiaScheduleTopFade"
            anchors.top: parent.top; anchors.left: parent.left
            width: scheduleListView.width; height: Math.min(28, scheduleListView.height / 4)
            z: 10
            opacity: scheduleListView.contentHeight > scheduleListView.height && !scheduleListView.atYBeginning ? 1 : 0
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.alpha(Kirigami.Theme.backgroundColor, 1) }
                GradientStop { position: 1; color: Qt.alpha(Kirigami.Theme.backgroundColor, 0) }
            }
            Behavior on opacity { NumberAnimation { duration: scheduleFadeTheme.duration } }
        }
        Rectangle {
            parent: scheduleListView
            objectName: "sequoiaScheduleBottomFade"
            anchors.bottom: parent.bottom; anchors.left: parent.left
            width: scheduleListView.width; height: Math.min(28, scheduleListView.height / 4)
            z: 10
            opacity: scheduleListView.contentHeight > scheduleListView.height && !scheduleListView.atYEnd ? 1 : 0
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.alpha(Kirigami.Theme.backgroundColor, 0) }
                GradientStop { position: 1; color: Qt.alpha(Kirigami.Theme.backgroundColor, 1) }
            }
            Behavior on opacity { NumberAnimation { duration: scheduleFadeTheme.duration } }
        }''')
        return replace(t, 'width: Math.min(parent.width, maxWidth)', 'width: parent.width')
    patch('MainViews/BasicMonthListView.qml', schedule_days)
    def schedule_event(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport org.sequoia.components as S\n')
        t = replace(t, '    id: incidenceCard',
                    '    id: incidenceCard\n    objectName: "sequoiaScheduleEventCard"\n    Layout.minimumWidth: 0')
        t = replace(t, '\n    Layout.fillWidth: true\n', '''
    Layout.fillWidth: false
    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
    readonly property real availableCardWidth: Math.max(0, scrollView.availableWidth - Kirigami.Units.gridUnit * 4 - Kirigami.Units.largeSpacing * 2 - Kirigami.Units.smallSpacing)
    Layout.preferredWidth: Math.min(availableCardWidth, Math.max(180, implicitWidth))
    Layout.maximumWidth: availableCardWidth
    leftPadding: 18
    rightPadding: 14
''')
        t = replace(t, '''    background: Calendar.IncidenceDelegateBackground {
        id: incidenceDelegateBackground
        isOpenOccurrence: incidenceCard.isOpenOccurrence
        isDark: scrollView.isDark
    }''', '''    background: Calendar.SequoiaScheduleEventSurface {
        calendarColor: modelData.color
        selected: incidenceCard.isOpenOccurrence
        hovered: incidenceMouseArea.containsMouse
        pressed: incidenceMouseArea.pressed
    }''')
        t = replace(t, 'property color textColor: Calendar.LabelUtils.getIncidenceLabelColor(incidenceCard.modelData.color, scrollView.isDark)',
                    'property color textColor: Kirigami.Theme.textColor')
        t = re.sub(r'color: incidenceCard.isOpenOccurrence \?\s*\(Calendar.LabelUtils.isDarkColor\(incidenceCard.modelData.color\) \? "white" : "black"\)\s*:\s*cardContents.textColor',
                   'color: cardContents.textColor', t)
        # Let the native AbstractCard derive its height from wrapped content.
        # Zero minimum widths keep long text from widening the schedule column.
        t = replace(t, '        ColumnLayout {\n            Layout.fillWidth: true',
                    '        ColumnLayout {\n            Layout.fillWidth: true\n            Layout.minimumWidth: 0')
        t = t.replace('            QQC2.Label {\n                Layout.fillWidth: true',
                      '            QQC2.Label {\n                Layout.fillWidth: true\n                Layout.minimumWidth: 0\n                wrapMode: Text.Wrap')
        t = t.replace('                elide: Text.ElideRight', '                elide: Text.ElideNone')
        t = replace(t, '                maximumLineCount: 3', '                Layout.minimumWidth: 0')
        t = replace(t, '                wrapMode: Text.WordWrap', '                wrapMode: Text.Wrap')
        return t
    patch('Delegates/MonthListViewIncidenceDelegate.qml', schedule_event)
    def tasks(t):
        t = replace(t, 'Kirigami.ScrollablePage {\n    id: root', 'Kirigami.Page {\n    id: root')
        t = replace(t, '    leftPadding: Kirigami.Units.largeSpacing', '    leftPadding: 16')
        start = t.index('    TodoTreeView {\n        id: incompleteView')
        content = t[start:t.rfind('}')]
        content = replace(content, '        id: incompleteView', '        id: incompleteView\n        objectName: "sequoiaTaskPageList"')
        content = replace(content, '        id: addField', '        id: addField\n        objectName: "sequoiaTaskPageAdd"')
        content = replace(content, '    footer: Kirigami.ActionTextField {', '''    Kirigami.ActionTextField {
        Layout.fillWidth: false
        Layout.alignment: Qt.AlignLeft
        Layout.preferredWidth: Math.min(taskPanel.width - 32, Math.max(textMetrics.advanceWidth(placeholderText), textMetrics.advanceWidth(text)) + leftPadding + rightPadding + 8)
        Layout.maximumWidth: Math.max(0, taskPanel.width - 32)
        leftPadding: 12; rightPadding: 40; topPadding: 10; bottomPadding: 10''')
        content = replace(content, '            id: textMetrics', '            id: textMetrics\n            font: addField.font')
        bg_start = content.index('        background: Rectangle {')
        bg_end = content.index('\n        function addTodo()', bg_start)
        content = content[:bg_start] + """        background: S.Panel {
            radius: 12
            color: Qt.alpha(taskPanel.theme.textColor, .035)
        }
""" + content[bg_end:]
        return t[:start] + """
    S.Panel {
        id: taskPanel
        objectName: "sequoiaMerkuroCalendar"
        anchors.fill: parent
        followMaterial: false; tint: 1
        Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 10
            Kirigami.Heading { text: qsTr("Tasks"); font.pixelSize: 16; font.weight: Font.DemiBold }
""" + content + '\n        }\n    }\n}\n'
    patch('MainViews/TodoView.qml', tasks)
    def task_tree(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport org.sequoia.components as S\n')
        t = replace(t, '    id: root', '    id: root\n    S.Theme { id: taskTheme }\n    spacing: 4\n    QQC2.ScrollBar.vertical: Calendar.SequoiaGlassScrollBar {}')
        t = replace(t, '        id: listItem', """        id: listItem
        topPadding: 8; bottomPadding: 8
        background: Rectangle {
            radius: 8
            color: listItem.hovered || listItem.visualFocus || listItem.highlighted ? Qt.alpha(taskTheme.accentColor, .08) : "transparent"
        }""")
        t = replace(t, '        Kirigami.Theme.highlightColor: Calendar.LabelUtils.getIncidenceDelegateBackgroundColor(color, root.isDark)', '        Kirigami.Theme.highlightColor: taskTheme.accentColor')
        start = t.index('                Calendar.TodoCheckBox {')
        end = t.index('\n                QQC2.Label {', start)
        t = t[:start] + """                QQC2.CheckBox {
                    id: taskCheck
                    objectName: "sequoiaTaskPageCheck"
                    Layout.row: 0; Layout.column: 0; Layout.rowSpan: 2
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    padding: 3
                    checked: listItem.todoCompleted || listItem.percent === 100
                    enabled: !listItem.isReadOnly
                    Accessible.name: qsTr("Complete %1").arg(listItem.model.text)
                    onClicked: listItem.model.checked = checked ? Qt.Checked : Qt.Unchecked
                    contentItem: Item {}
                    indicator: Rectangle {
                        x: 3; y: 3; width: 18; height: 18; radius: 4
                        color: taskCheck.checked ? taskTheme.accentColor : "transparent"
                        border.width: taskCheck.visualFocus ? 2 : 1
                        border.color: taskCheck.checked || taskCheck.visualFocus ? taskTheme.accentColor : Qt.alpha(taskTheme.textColor, .5)
                        Kirigami.Icon { anchors.centerIn: parent; width: 13; height: 13; source: "checkmark"; color: "white"; visible: taskCheck.checked }
                    }
                }
""" + t[end:]
        return t
    patch('MainViews/TodoTreeView.qml', task_tree)

    def main(t):
        t = replace(t, '    id: root', """    id: root
    readonly property bool sequoiaMakeover: true
    contentItem.anchors.topMargin: (header?.visible ? header.height : 0)
        + (menuBar?.visible ? menuBar.height : 0)
    pageStack.globalToolBar.style: pageStack.currentItem?.sequoiaMainPage
        ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ToolBar
    function showSequoiaToday() {
        DateTimeState.resetTime();
        root.switchView(hourlyViewComponent, { daysToShow: 1, sequoiaToday: true, createEventAction: root.createAction });
    }
""")
        t = replace(t, '        if (root.filterHeaderBarLoaderItem.active) {',
                    '        if (root.filterHeaderBarLoaderItem.active && !root.pageStack.currentItem.sequoiaMainPage) {')
        t = replace(t, '                filterHeaderBar.active = true;', '                filterHeaderBar.active = false;')
        t = replace(t, '    globalDrawer: MainDrawer {', '    globalDrawer: SequoiaSidebar {')
        # Day -> Week etc must also leave the Today dashboard when modes match.
        t = t.replace(' || root.ignoreCurrentPage)', ' || root.ignoreCurrentPage || root.pageStack.currentItem.sequoiaToday)')
        return t
    patch('Main.qml', main)

    def month_grid(t):
        t = replace(t, 'property int spacing: Calendar.Config.monthGridBorderWidth',
                    'property int spacing: Math.max(1, Calendar.Config.monthGridBorderWidth)')
        # A themed Separator may paint the same color as the day cells. Give
        # the gaps a predictable palette-relative color in both color schemes.
        t = replace(t, '    Kirigami.Separator {\n        id: gridBackground',
                    '''    Rectangle {
        id: gridBackground
        color: Qt.alpha(Kirigami.Theme.textColor, 0.22)''')
        return t[:t.rfind('}')] + '''
    Calendar.SequoiaMonthFrame {
        anchors.fill: parent
        z: 100
    }
}
'''
    patch('MainViews/DayGridView.qml', month_grid)
    def today_timeline(t):
        t = replace(t, '    property bool dragDropEnabled: true', '    property bool dragDropEnabled: true\n    property bool sequoiaCompactHeader: false')
        t = replace(t, '        id: headingRow', '        id: headingRow\n        visible: !viewColumn.sequoiaCompactHeader\n        height: visible ? implicitHeight : 0')
        t = replace(t, 'readonly property real periodHeight: Kirigami.Units.gridUnit',
                    'readonly property real periodHeight: sequoiaCompactHeader ? Math.max(13.5, Math.floor((height - (allDayHeader.visible ? allDayHeader.height : 0) - 24) / 12.5) / 4) : Kirigami.Units.gridUnit')
        t = replace(t, 'readonly property real hourLabelWidth: hourLabelMetrics', 'readonly property real hourLabelWidth: sequoiaCompactHeader ? 52 : hourLabelMetrics')
        t = t.replace('viewColumn.scrollbarWidth = hourlyView.QQC2.ScrollBar.vertical.width', 'viewColumn.scrollbarWidth = viewColumn.sequoiaCompactHeader ? 0 : hourlyView.QQC2.ScrollBar.vertical.width')
        t = replace(t, '        z: -2\n        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff',
                    '        z: -2\n        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff\n        QQC2.ScrollBar.vertical.policy: viewColumn.sequoiaCompactHeader ? QQC2.ScrollBar.AlwaysOff : QQC2.ScrollBar.AsNeeded')
        t = replace(t, '            setPosition(Math.max(0, (new Date()).getHours() - 1) / 23);',
                    '            setPosition(viewColumn.sequoiaCompactHeader ? (8 * (hourlyView.hourHeight + viewColumn.gridLineWidth) - 10) / hourlyView.dayHeight : Math.max(0, (new Date()).getHours() - 1) / 23);')
        t = replace(t, '        readonly property Item vScrollBar: QQC2.ScrollBar.vertical',
                    '        readonly property Item vScrollBar: QQC2.ScrollBar.vertical\n        onActualHeightChanged: if (viewColumn.sequoiaCompactHeader) Qt.callLater(setToCurrentTime)')
        t = replace(t, '                    active: currentTimeMarkerLoader.active', '                    active: !viewColumn.sequoiaCompactHeader && currentTimeMarkerLoader.active')
        t = replace(t, '                        text: modelData\n                        color: Kirigami.Theme.disabledTextColor',
                    '                        text: viewColumn.sequoiaCompactHeader ? String(index + 1).padStart(2, "0") + ":00" : modelData\n                        font.pixelSize: viewColumn.sequoiaCompactHeader ? 12 : Kirigami.Theme.defaultFont.pixelSize\n                        color: Kirigami.Theme.disabledTextColor')
        t = replace(t, '            Item {\n                id: innerWeekView', '''            Rectangle {
                visible: viewColumn.sequoiaCompactHeader
                anchors.left: hourLabelsColumn.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 1; z: 1; color: Qt.alpha(Kirigami.Theme.textColor, .08)
            }
            Item {
                id: innerWeekView''')
        t = replace(t, '                Kirigami.Separator {\n                    anchors.fill: parent\n                }',
                    '                Rectangle {\n                    objectName: "sequoiaHourlyLineGrid"\n                    anchors.fill: parent\n                    color: Qt.alpha(Kirigami.Theme.textColor, viewColumn.sequoiaCompactHeader ? .12 : .22)\n                }')
        t = replace(t, '                                        required property int index\n                                        readonly property color bgColor: {',
                    '                                        required property int index\n                                        readonly property color bgColor: {\n                                            if (viewColumn.sequoiaCompactHeader) return Qt.alpha(Kirigami.Theme.backgroundColor, 1);')
        t = t.replace('return Kirigami.Theme.activeBackgroundColor', 'return Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.highlightColor, 0.055)')
        # Today all-day rows use the same event skin, with enough height for
        # title, All day, and the calendar label. Month/Week keep native sizing.
        t = replace(t, 'property int allDayViewDelegateHeight: Kirigami.Units.gridUnit + Kirigami.Units.smallSpacing',
                    'property int allDayViewDelegateHeight: sequoiaCompactHeader ? 58 : Kirigami.Units.gridUnit + Kirigami.Units.smallSpacing')
        t = replace(t, '        property int actualHeight: {\n            if (Calendar.Config.weekViewAllDayHeaderHeight',
                    '        property int actualHeight: {\n            if (viewColumn.sequoiaCompactHeader) return defaultHeight;\n            if (Calendar.Config.weekViewAllDayHeaderHeight')
        t = replace(t, '            text: i18n("All day or Multi day")', '            text: viewColumn.sequoiaCompactHeader ? i18n("All day") : i18n("All day or Multi day")\n            Binding on leftPadding { when: viewColumn.sequoiaCompactHeader; value: 0 }')
        t = replace(t, '                                        id: linesRepeater', '                                        id: linesRepeater\n                                        objectName: viewColumn.sequoiaCompactHeader ? "sequoiaAllDayRows" : ""')
        t = replace(t, '                                            height: Kirigami.Units.gridUnit + Kirigami.Units.smallSpacing',
                    '                                            height: viewColumn.allDayViewDelegateHeight')
        t = replace(t, '                                                    id: incidenceDelegate',
                    '                                                    id: incidenceDelegate\n                                                    sequoiaTodayEvent: viewColumn.sequoiaCompactHeader')
        t = replace(t, '                                                color: multiDayViewBackground.bgColor',
                    '                                                color: viewColumn.sequoiaCompactHeader ? Qt.alpha(Kirigami.Theme.backgroundColor, 1) : multiDayViewBackground.bgColor')
        # ScrollBar.position can be reset by the underlying Flickable when
        # all-day rows arrive asynchronously. Position the actual viewport
        # after its layout settles, and repeat only on geometry/date changes.
        t = replace(t, '        function setToCurrentTime(): void {', '''        Timer {
            id: sequoiaPositionTimer
            interval: 80
            onTriggered: {
                const viewport = hourlyView.contentItem;
                if (viewport && viewport.contentY !== undefined) {
                    viewport.contentY = Math.min(Math.max(0, viewport.contentHeight - viewport.height),
                        8 * (hourlyView.hourHeight + viewColumn.gridLineWidth) - 10);
                }
            }
        }
        Connections {
            target: hourlyView.contentItem
            function onContentHeightChanged() { if (viewColumn.sequoiaCompactHeader) sequoiaPositionTimer.restart(); }
        }
        function setToCurrentTime(): void {
            if (viewColumn.sequoiaCompactHeader) { sequoiaPositionTimer.restart(); return; }''')
        # The native wrappers still own event positioning, overlap, interaction,
        # drag/drop, and resizing. Only their Today presentation is replaced.
        t = replace(t, '                                    id: hourlyIncidencesRepeater',
                    '                                    id: hourlyIncidencesRepeater\n                                    objectName: viewColumn.sequoiaCompactHeader ? "sequoiaTimelineEvents" : ""')
        t = replace(t, '                                            id: incidenceDelegateBackground\n                                            isOpenOccurrence:',
                    '                                            id: incidenceDelegateBackground\n                                            visible: !viewColumn.sequoiaCompactHeader\n                                            isOpenOccurrence:')
        t = replace(t, '                                            id: incidenceContents',
                    '                                            id: incidenceContents\n                                            visible: !viewColumn.sequoiaCompactHeader')
        t = replace(t, '                                        Calendar.IncidenceMouseArea {\n                                            id: mouseArea', '''                                        Calendar.SequoiaTimelineEvent {
                                            anchors.fill: parent
                                            visible: viewColumn.sequoiaCompactHeader
                                            title: modelData.text
                                            startTime: modelData.startTime
                                            endTime: modelData.endTime
                                            calendarColor: modelData.color
                                            calendarName: modelData.collectionId > 0 ? (Calendar.CalendarManager.getCollectionDetails(modelData.collectionId)?.displayName ?? "") : ""
                                            selected: parent.isOpenOccurrence
                                        }

                                        Calendar.IncidenceMouseArea {
                                            id: mouseArea''')
        t = replace(t, '                        height: viewColumn.gridLineWidth * 2\n                        color: Kirigami.Theme.highlightColor',
                    '                        height: viewColumn.sequoiaCompactHeader ? 1 : viewColumn.gridLineWidth * 2\n                        color: viewColumn.sequoiaCompactHeader ? "#246bff" : Kirigami.Theme.highlightColor')
        t = replace(t, '                            height: parent.height * 5\n                            radius: 100\n                            color: Kirigami.Theme.highlightColor',
                    '                            height: viewColumn.sequoiaCompactHeader ? 8 : parent.height * 5\n                            radius: 100\n                            color: parent.color')
        t = replace(t, '                        id: currentTimeMarker\n', '''                        id: currentTimeMarker
                        QQC2.Label {
                            objectName: "sequoiaTodayTimePill"
                            visible: viewColumn.sequoiaCompactHeader
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: Qt.formatTime(viewColumn.currentDate, "HH:mm")
                            color: "white"; font.pixelSize: 11; font.weight: Font.DemiBold
                            leftPadding: 7; rightPadding: 7; topPadding: 2; bottomPadding: 2
                            background: Rectangle { radius: height / 2; color: "#246bff" }
                        }
''')
        t = replace(t, '        id: headerTopSeparator\n', '        id: headerTopSeparator\n        visible: !viewColumn.sequoiaCompactHeader\n')
        t = replace(t, '        id: headerBottomShadow\n', '        id: headerBottomShadow\n        visible: !viewColumn.sequoiaCompactHeader\n')
        return t
    def hourly_scrollbars(t):
        t = today_timeline(t)
        t = replace(t, '        function setToCurrentTime(): void {', '        function setToCurrentTime(): void {\n            if (viewColumn.sequoiaScrollManaged) return;')
        t = replace(t, '        function setPosition(position: real): void {', '        function setPosition(position: real): void {\n            if (viewColumn.sequoiaScrollManaged) return;')
        t = replace(t, '        function currentPosition(): real {', '        function sequoiaStopScrollAnimation() { scrollAnimation.stop(); }\n        function currentPosition(): real {')
        t = replace(t, '    property bool sequoiaCompactHeader: false', """    property bool sequoiaCompactHeader: false
    property bool sequoiaSynchronized: false
    property bool sequoiaScrollManaged: false
    property real sequoiaHeaderPadding: 0
    readonly property real sequoiaHeaderHeight: (headingRow.visible ? headingRow.height : 0)
        + (holidayRow.visible ? holidayRow.height : 0)
        + (headerTopSeparator.visible ? headerTopSeparator.height : 0)
        + (allDayHeader.visible ? allDayHeader.height : 0)
        + (headerBottomSeparator.visible ? headerBottomSeparator.height : 0)""")
        t = replace(t, '    QQC2.ScrollView {\n        id: hourlyView', """    Item {
        width: viewColumn.width
        height: viewColumn.sequoiaHeaderPadding
    }
    QQC2.ScrollView {
        id: hourlyView""")
        t = replace(t, '        property int actualHeight: {\n            let h =', '        property int actualHeight: {\n            if (viewColumn.sequoiaSynchronized) return Math.max(0, viewColumn.height - viewColumn.sequoiaHeaderHeight - viewColumn.sequoiaHeaderPadding);\n            let h =')
        t = replace(t, '        QQC2.ScrollBar.vertical.policy: viewColumn.sequoiaCompactHeader ? QQC2.ScrollBar.AlwaysOff : QQC2.ScrollBar.AsNeeded', """        QQC2.ScrollBar.vertical: Calendar.SequoiaGlassScrollBar {
            policy: viewColumn.sequoiaCompactHeader ? QQC2.ScrollBar.AlwaysOff : QQC2.ScrollBar.AsNeeded
            parent: hourlyView
            x: hourlyView.width - width
            y: hourlyView.topPadding
            height: hourlyView.availableHeight
            z: 20
        }""")
        return t
    patch('MainViews/BasicInternalHourlyView.qml', hourly_scrollbars)
    def today_all_day(t):
        t = replace(t, '    property int month: 0', '    property bool sequoiaTodayEvent: false\n    property int month: 0')
        t = replace(t, '        id: incidenceDelegateBackground', '        id: incidenceDelegateBackground\n        visible: !incidenceDelegate.sequoiaTodayEvent')
        t = replace(t, '        id: incidenceContents', '        id: incidenceContents\n        visible: !incidenceDelegate.sequoiaTodayEvent')
        t = replace(t, '    Calendar.IncidenceMouseArea {', '''    Loader {
        anchors.fill: parent
        active: incidenceDelegate.sequoiaTodayEvent
        sourceComponent: Calendar.SequoiaTimelineEvent {
            title: modelData.text
            startTime: incidenceDelegate.occurrenceDate
            endTime: incidenceDelegate.occurrenceEndDate
            allDay: incidenceDelegate.allDay
            calendarColor: modelData.color
            calendarName: modelData.collectionId > 0 ? (Calendar.CalendarManager.getCollectionDetails(modelData.collectionId)?.displayName ?? "") : ""
            selected: incidenceDelegate.isOpenOccurrence
        }
    }
    Calendar.IncidenceMouseArea {''')
        return t
    patch('Delegates/DayGridViewIncidenceDelegate.qml', today_all_day)

    def calendar_navigation(t):
        def object_end(text, start):
            brace = text.index('{', start)
            depth = 0
            for match in re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/|[{}]', text[brace:], re.S):
                if match.group() == '{': depth += 1
                elif match.group() == '}':
                    depth -= 1
                    if depth == 0: return brace + match.end()
            raise RuntimeError('Unbalanced calendar navigation object')
        # Use the native check-state handlers and move each complete control
        # before its label. A fixed-size circle replaces the decorative icon.
        for owner in ('collectionSourceItem', 'collectionItem'):
            owner_start = t.index('id: ' + owner + '\n')
            row_start = t.index('contentItem: RowLayout {', owner_start)
            check_start = t.index('ColoredCheckbox {', row_start)
            check_end = object_end(t, check_start)
            checkbox = t[check_start:check_end].replace('ColoredCheckbox {', 'SequoiaCalendarIndicator {\n                                text: ' + owner + '.text')
            t = t[:check_start] + t[check_end:]
            icon_start = t.index('Kirigami.Icon {', row_start)
            icon_end = object_end(t, icon_start)
            # Keep the leading column even for non-checkable account sources.
            # Hiding their checkbox must not pull the account address left.
            checkbox = checkbox.replace('SequoiaCalendarIndicator {', 'SequoiaCalendarIndicator {\n                                anchors.fill: parent', 1)
            leading = ('Item {\n                                Layout.preferredWidth: 26\n'
                       '                                Layout.preferredHeight: 28\n'
                       '                                Layout.alignment: Qt.AlignVCenter\n'
                       '                                ' + checkbox + '\n'
                       '                                Kirigami.Icon {\n'
                       '                                    anchors.centerIn: parent\n'
                       '                                    width: Kirigami.Units.iconSizes.smallMedium; height: width\n'
                       '                                    visible: !collectionCheckbox.visible\n'
                       '                                    source: "user-identity"\n'
                       '                                    isMask: true\n'
                       '                                    color: Kirigami.Theme.disabledTextColor\n'
                       '                                }\n                            }')
            t = t[:icon_start] + leading + t[icon_end:]
        # Account and calendar names share the heading's text column. The
        # circle is slightly wider than its icon, so account for that width.
        t = re.sub(r'(?m)^(                        )leftInset: .*$', r'\1leftInset: collectionHeadingItem.leftInset', t)
        t = re.sub(r'(?m)^(                        )leftPadding: .*$',
            r'\1leftPadding: Math.max(0, collectionHeadingItem.leftPadding + Kirigami.Units.iconSizes.smallMedium - collectionCheckbox.implicitWidth)', t)
        t = t.replace('(kDescendantLevel - 1) * padding * 2', 'Math.max(0, kDescendantLevel - 1) * padding * 2')
        # Both section headings and expandable calendar sources get the same
        # small glass circle and V-shaped chevron.
        for owner, expanded, action in (
            ('tagsHeadingItem', 'tagsHeadingItem.expanded', 'tagsHeadingItem.clicked()'),
            ('collectionHeadingItem', 'collectionHeadingItem.expanded', 'collectionHeadingItem.clicked()'),
            ('collectionSourceItem', 'collectionSourceItem.kDescendantExpanded', 'collectionList.model.toggleChildren(collectionSourceItem.index)'),
        ):
            marker = t.index("source: " + expanded + " ? 'arrow-up' : 'arrow-down'")
            start = t.rfind('Kirigami.Icon {', 0, marker)
            end = object_end(t, start)
            t = t[:start] + ('SequoiaCalendarChevron {\n'
                '                    objectName: "sequoiaCalendarChevron-' + owner + '"\n'
                '                    expanded: ' + expanded + '\n'
                '                    onClicked: ' + action + '\n'
                '                    Layout.alignment: Qt.AlignVCenter\n'
                '                }') + t[end:]
        # Parent TapHandlers also see taps over child buttons. Keep those taps
        # from expanding the source or selecting a calendar a second time.
        t = replace(t, 'objectName: "sequoiaCalendarChevron-collectionSourceItem"',
                    'id: sourceExpandControl\n                    objectName: "sequoiaCalendarChevron-collectionSourceItem"')
        t = replace(t, 'onLeftClicked: collectionList.model.toggleChildren(index)', '''onLeftClicked: {
                                function over(control) {
                                    const p = control.mapFromItem(tapHandler.parent, tapHandler.point.position.x, tapHandler.point.position.y);
                                    return p.x >= 0 && p.y >= 0 && p.x < control.width && p.y < control.height;
                                }
                                if (!over(collectionCheckbox) && !over(sourceExpandControl)) collectionList.model.toggleChildren(index);
                            }''')
        t = replace(t, '                            onLeftClicked: {\n                                Filter.collectionId', '''                            onLeftClicked: {
                                const p = collectionCheckbox.mapFromItem(tapHandler.parent, tapHandler.point.position.x, tapHandler.point.position.y);
                                if (p.x >= 0 && p.y >= 0 && p.x < collectionCheckbox.width && p.y < collectionCheckbox.height) return;
                                Filter.collectionId''')
        # A source stays expanded in the native tree until its children have
        # finished closing. Reversing the animation therefore keeps the same
        # delegates and check states instead of removing and recreating them.
        t = t.replace('collectionList.model.toggleChildren(collectionSourceItem.index)',
                      'collectionSourceItem.toggleExpanded()')
        t = t.replace('if (!over(collectionCheckbox) && !over(sourceExpandControl)) collectionList.model.toggleChildren(index);',
                      'if (!over(collectionCheckbox) && !over(sourceExpandControl)) collectionSourceItem.toggleExpanded();')
        t = t.replace('expanded: collectionSourceItem.kDescendantExpanded',
                      'expanded: collectionSourceItem.expandedWanted')
        t = t.replace('Accessible.checked: collectionSourceItem.kDescendantExpanded',
                      'Accessible.checked: collectionSourceItem.expandedWanted')
        t = replace(t, 'id: collectionSourceItem\n', '''id: collectionSourceItem
                        property bool expandedWanted: kDescendantExpanded
                        property real childrenReveal: expandedWanted ? 1 : 0
                        onKDescendantExpandedChanged: expandedWanted = kDescendantExpanded
                        function toggleExpanded() {
                            const opening = !expandedWanted;
                            if (opening && !kDescendantExpanded)
                                collectionList.model.toggleChildren(index);
                            expandedWanted = opening;
                        }
                        onChildrenRevealChanged: {
                            if (childrenReveal === 0 && !expandedWanted && kDescendantExpanded)
                                collectionList.model.toggleChildren(index);
                        }
                        Behavior on childrenReveal {
                            NumberAnimation { duration: navigationTheme.duration * 2; easing.type: Easing.InOutCubic }
                        }
''')
        for owner in ('collectionSourceItem', 'collectionItem'):
            marker = t.index('id: ' + owner + '\n')
            start = t.rfind('Delegates.RoundedItemDelegate {', 0, marker)
            end = object_end(t, start)
            delegate = t[start:end]
            properties = re.findall(r'                        required property [^\n]+', delegate)
            wrapper = owner + 'Row'
            for prop in properties:
                name = prop.split()[-1]
                delegate = delegate.replace(prop, '                        property alias ' + name + ': ' + wrapper + '.' + name)
            source = owner == 'collectionSourceItem'
            margin = '2 * Kirigami.Units.largeSpacing' if source else '0'
            delegate = delegate.replace('                        Layout.topMargin: 2 * Kirigami.Units.largeSpacing\n', '')
            delegate = delegate.replace('                        Layout.fillWidth: true',
                '                        width: parent.width\n                        height: implicitHeight\n                        y: ' + margin, 1)
            t = t[:start] + ('Item {\n                        id: ' + wrapper + '\n'
                '                        objectName: "sequoiaCalendarAccountRow"\n' + '\n'.join(properties) + '\n'
                '                        readonly property real childrenReveal: ' + (owner + '.childrenReveal' if source else '1') + '\n'
                '                        readonly property real ancestorReveal: collectionList.ancestorReveal(index, kDescendantLevel)\n'
                '                        implicitHeight: (' + owner + '.implicitHeight + ' + margin + ') * ancestorReveal\n'
                '                        Layout.fillWidth: true\n'
                '                        Layout.minimumHeight: implicitHeight\n'
                '                        Layout.maximumHeight: implicitHeight\n'
                '                        clip: true\n'
                '                        opacity: ancestorReveal\n'
                '                        visible: ancestorReveal > 0\n'
                '                        enabled: ancestorReveal === 1\n'
                '                        ' + delegate + '\n                    }') + t[end:]
        t = replace(t, 'id: collectionList\n', '''id: collectionList
            property int rowsRevision: 0
            onItemAdded: rowsRevision++
            onItemRemoved: rowsRevision++
            function ancestorReveal(row, level) {
                // Reading the revision also tracks delegates added after a
                // parent is expanded, including nested calendar sources.
                const revision = rowsRevision;
                let reveal = 1;
                for (let i = row - 1; i >= 0 && level > 0; --i) {
                    const ancestor = itemAt(i);
                    if (ancestor && ancestor.kDescendantLevel < level) {
                        reveal *= ancestor.childrenReveal;
                        level = ancestor.kDescendantLevel;
                    }
                }
                return reveal;
            }
''')
        # Keep the native model alive while the section closes. Swapping it
        # for [] destroys the rows before any closing transition can run.
        t = replace(t, 'import QtQuick.Layouts', 'import QtQuick.Layouts\nimport org.sequoia.components as S')
        start = t.index('        Repeater {\n            id: collectionList')
        end = object_end(t, start)
        rows = t[start:end].replace('model: collectionHeadingItem.expanded ? collectionModel : []',
                                    'model: collectionModel')
        t = t[:start] + '''        Item {
            id: calendarSection
            objectName: "sequoiaCalendarSection"
            Layout.fillWidth: true
            Layout.minimumHeight: implicitHeight
            Layout.maximumHeight: implicitHeight
            implicitHeight: calendarRows.implicitHeight * reveal
            clip: true
            visible: reveal > 0
            enabled: collectionHeadingItem.expanded
            property real reveal: collectionHeadingItem.expanded ? 1 : 0
            S.Theme { id: navigationTheme }
            Behavior on reveal {
                NumberAnimation { duration: navigationTheme.duration * 2; easing.type: Easing.InOutCubic }
            }
            ColumnLayout {
                id: calendarRows
                width: parent.width
                spacing: 0
                opacity: calendarSection.reveal
''' + rows + '''
            }
        }''' + t[end:]
        return t
    patch('Controls/CheckableCollectionNavigationView.qml', calendar_navigation)

    patch('Controls/DayTapHandler.qml', lambda t: replace(t, '            deselect();', '''            if (!includeTime && Calendar.CalendarUiUtils.appMain?.pageStack.currentItem?.mode === Calendar.CalendarApplication.Month)
                Calendar.DateTimeState.selectedDate = addDate;
            deselect();'''))
    patch('MainViews/DayGridViewBackground.qml', lambda t: replace(replace(replace(t,
        '                                    flat: true',
        '''                                    flat: true
                                    background: Item {}'''),
        'onClicked: Calendar.CalendarUiUtils.openDayLayer(gridItem.date)',
        '''onClicked: {
                                        if (Calendar.CalendarUiUtils.appMain.pageStack.currentItem.width >= 820)
                                            Calendar.DateTimeState.selectedDate = gridItem.date;
                                        else Calendar.CalendarUiUtils.openDayLayer(gridItem.date);
                                    }'''),
        '                                    color: gridItem.bgColor',
        '''                                    color: gridItem.bgColor
                                    Rectangle {
                                        objectName: "sequoiaMonthDaySelection"
                                        anchors.fill: parent; anchors.margins: 2; radius: 0
                                        color: "transparent"; border.width: 1
                                        border.color: Kirigami.Theme.highlightColor
                                        visible: Calendar.DateUtils.sameDay(gridItem.date, Calendar.DateTimeState.selectedDate)
                                    }'''))
