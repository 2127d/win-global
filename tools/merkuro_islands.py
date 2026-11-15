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
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Qml'], text=True))
    subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(bridge_source), '-o',
                    str(destination / 'merkuro-qml.so'), *flags, '-ldl'], check=True)
    qmldir = module / 'qmldir'
    qmldir.write_text(''.join(line for line in qmldir.read_text().splitlines(True) if not line.startswith('prefer ')))

    def patch(relative, transform):
        path = module / 'qml' / relative
        path.write_text(transform(path.read_text()))

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

    patch('MainViews/MonthView.qml', lambda t: wrap_native(t, """
        Calendar.SequoiaAgenda {
            visible: root.width >= 820
            Layout.preferredWidth: Math.min(320, root.width * .29)
            Layout.maximumWidth: 320; Layout.fillWidth: false
            Layout.fillHeight: true
        }
"""))
    def hourly(t):
        t = wrap_native(t, '', '    property bool sequoiaToday: false\n')
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
    patch('MainViews/ScheduleView.qml', lambda t: wrap_native(t, ''))
    # ScrollablePage owns its own viewport; inset the card below its header.
    patch('MainViews/TodoView.qml', lambda t: replace(t, '    background: Item {}', """    background: Item {
        S.Panel {
            objectName: "sequoiaMerkuroCalendar"
            anchors.fill: parent; anchors.margins: 8
            followMaterial: false; tint: 1
            Kirigami.Theme.inherit: false; Kirigami.Theme.colorSet: Kirigami.Theme.View
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
        }
    }"""))

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

    patch('MainViews/DayGridView.qml', lambda t: replace(t,
        'property int spacing: Calendar.Config.monthGridBorderWidth',
        'property int spacing: Math.max(1, Calendar.Config.monthGridBorderWidth)'))
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
                    '                Kirigami.Separator {\n                    anchors.fill: parent\n                    Binding on color { when: viewColumn.sequoiaCompactHeader; value: Qt.alpha(Kirigami.Theme.textColor, .12) }\n                }')
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
    patch('MainViews/BasicInternalHourlyView.qml', today_timeline)
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
        return t
    patch('Controls/CheckableCollectionNavigationView.qml', calendar_navigation)

    patch('Controls/DayTapHandler.qml', lambda t: replace(t, '            deselect();', '''            if (!includeTime && Calendar.CalendarUiUtils.appMain?.pageStack.currentItem?.mode === Calendar.CalendarApplication.Month)
                Calendar.DateTimeState.selectedDate = addDate;
            deselect();'''))
    patch('MainViews/DayGridViewBackground.qml', lambda t: replace(replace(t,
        'onClicked: Calendar.CalendarUiUtils.openDayLayer(gridItem.date)',
        '''onClicked: {
                                        if (Calendar.CalendarUiUtils.appMain.pageStack.currentItem.width >= 820)
                                            Calendar.DateTimeState.selectedDate = gridItem.date;
                                        else Calendar.CalendarUiUtils.openDayLayer(gridItem.date);
                                    }'''),
        '                                    color: gridItem.bgColor',
        '''                                    color: gridItem.bgColor
                                    Rectangle {
                                        anchors.fill: parent; anchors.margins: 2; radius: 8
                                        color: "transparent"; border.width: 1
                                        border.color: Kirigami.Theme.highlightColor
                                        visible: Calendar.DateUtils.sameDay(gridItem.date, Calendar.DateTimeState.selectedDate)
                                    }'''))
