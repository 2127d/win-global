/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "calendar" as SequoiaCalendar

import org.kde.plasma.plasmoid
import org.kde.ksvg as KSvg
import org.kde.plasma.workspace.calendar as PlasmaCalendar
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.private.digitalclock
import org.kde.config as KConfig
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.plasma.clock

// Top-level layout containing:
// - Leading column with world clock and agenda view
// - Trailing column with current date header and calendar
//
// The columns share the space beneath the live clock header, with inset cards.
// Representation's header is intentionally zero-sized, because Calendar view
// brings its own header, and there's currently no other way to stack them.
PlasmaExtras.Representation {
    id: calendar
    objectName: "sequoiaCalendar"

    readonly property var appletInterface: root

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    Layout.minimumWidth: (calendar.showAgenda || calendar.showClocks) ? Kirigami.Units.gridUnit * 46 : Kirigami.Units.gridUnit * 24
    Layout.maximumWidth: Kirigami.Units.gridUnit * 80

    Layout.minimumHeight: Kirigami.Units.gridUnit * 30
    Layout.maximumHeight: Kirigami.Units.gridUnit * 40

    collapseMarginsHint: true

    readonly property int paddings: Kirigami.Units.largeSpacing
    readonly property color cardColor: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.025)
    readonly property color cardBorder: Qt.alpha(Kirigami.Theme.textColor, 0.08)
    readonly property bool showAgenda: eventPluginsManager.enabledPlugins.length > 0
    readonly property bool showClocks: clocksList.count > 1

    readonly property alias monthView: monthView
    // This helps synchronize the header of the agenda and the monthView.
    // We cannot use Kirigami.SizeGroup here because monthView's header is not in a layout.
    readonly property double headerHeight: Math.max(agendaHeader.implicitHeight, monthView.viewHeader.implicitHeight)

    Keys.onDownPressed: event => {
        monthView.Keys.downPressed(event);
    }

    Connections {
        target: root

        function onExpandedChanged() {
            // clear all the selections when the plasmoid is showing/hiding
            monthView.resetToToday();
        }
    }

    PlasmaCalendar.EventPluginsManager {
        id: eventPluginsManager
        enabledPlugins: Plasmoid.configuration.enabledCalendarPlugins
    }

    // Having this in place helps preserving top margins for Pin and Configure
    // buttons somehow. Actual headers are spread across leading and trailing
    // columns.
    header: Item {}

    RowLayout {
        id: clockHeader
        anchors { top: parent.top; left: parent.left; right: parent.right; margins: calendar.paddings }
        height: Kirigami.Units.gridUnit * 4
        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            PlasmaComponents.Label {
                objectName: "sequoiaHeroTime"
                Layout.fillWidth: true
                text: root.formatTime(headerClock.dateTime, Plasmoid.configuration.showSeconds === 2)
                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 2.8
                font.weight: Font.Light
                font.features: { "tnum": 1 }
                elide: Text.ElideRight
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                text: Qt.formatDate(headerClock.dateTime, Qt.locale(), Locale.LongFormat)
                color: Kirigami.Theme.disabledTextColor
                elide: Text.ElideRight
            }
        }
        Rectangle {
            implicitWidth: Kirigami.Units.gridUnit * 2.8
            implicitHeight: Kirigami.Units.gridUnit * 2.8
            radius: 14
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
            Column {
                anchors.centerIn: parent
                PlasmaComponents.Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Qt.formatDate(headerClock.dateTime, "ddd")
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    color: Kirigami.Theme.linkColor
                }
                PlasmaComponents.Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Qt.formatDate(headerClock.dateTime, "d")
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.5
                    font.weight: Font.DemiBold
                    color: Kirigami.Theme.linkColor
                }
            }
        }
        Clock {
            id: headerClock
            timeZone: Plasmoid.configuration.lastSelectedTimezone
            trackSeconds: Plasmoid.configuration.showSeconds === 2
        }
    }

    Rectangle {
        anchors.fill: leadingColumn
        anchors.margins: -calendar.paddings
        visible: leadingColumn.visible
        radius: 18
        color: calendar.cardColor
        border.color: calendar.cardBorder
    }
    Kirigami.ShadowedRectangle {
        objectName: "sequoiaCalendarSurface"
        anchors.fill: monthViewWrapper
        radius: 18
        color: calendar.cardColor
        shadow.size: 14
        shadow.yOffset: 3
        shadow.color: Qt.rgba(0, 0, 0, 0.18)
    }

    // Leading column containing agenda view and time zones
    // ==================================================
    ColumnLayout {
        id: leadingColumn

        visible: calendar.showAgenda || calendar.showClocks

        anchors {
            top: clockHeader.bottom
            topMargin: calendar.paddings * 2
            left: parent.left
            leftMargin: calendar.paddings * 2
            right: monthViewWrapper.left
            rightMargin: calendar.paddings * 2
            bottom: parent.bottom
            bottomMargin: calendar.paddings * 2
        }

        spacing: 0

        PlasmaExtras.PlasmoidHeading {
            id: agendaHeader
            visible: calendar.showAgenda
            background: Item {}
            Layout.preferredHeight: calendar.headerHeight
            Layout.fillWidth: true
            leftInset: 0
            rightInset: 0

            // Agenda view header
            // -----------------
            contentItem: ColumnLayout {
                spacing: 0

                Kirigami.Heading {
                    Layout.alignment: Qt.AlignTop
                    // Match calendar title
                    Layout.leftMargin: calendar.paddings
                    Layout.rightMargin: calendar.paddings
                    Layout.fillWidth: true

                    text: monthView.currentDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                    textFormat: Text.PlainText
                }

                PlasmaComponents.Label {
                    visible: monthView.currentDateAuxilliaryText.length > 0

                    Layout.leftMargin: calendar.paddings
                    Layout.rightMargin: calendar.paddings
                    Layout.fillWidth: true

                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    text: monthView.currentDateAuxilliaryText
                    textFormat: Text.PlainText
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Layout.alignment: Qt.AlignBottom

                    // Heading text
                    Kirigami.Heading {
                        visible: agenda.visible

                        Layout.fillWidth: true
                        Layout.leftMargin: calendar.paddings
                        Layout.rightMargin: calendar.paddings

                        level: 2

                        text: i18nd("plasma_applet_org.kde.plasma.digitalclock", "Events")
                        textFormat: Text.PlainText
                        maximumLineCount: 1
                        elide: Text.ElideRight
                    }
                    PlasmaComponents.ToolButton {
                        id: addEventButton

                        visible: agenda.visible && ApplicationIntegration.calendarInstalled
                        text: i18ndc("plasma_applet_org.kde.plasma.digitalclock", "@action:button Add event", "Add…")
                        Layout.rightMargin: Kirigami.Units.smallSpacing
                        icon.name: "list-add"

                        Accessible.description: i18ndc("plasma_applet_org.kde.plasma.digitalclock", "@info:tooltip", "Add a new event")
                        KeyNavigation.down: KeyNavigation.tab
                        KeyNavigation.right: monthView.viewHeader.tabBar

                        onClicked: ApplicationIntegration.launchCalendar()
                        KeyNavigation.tab: calendar.showAgenda && eventsList.count ? eventsList : eventsList.KeyNavigation.down
                    }
                }
            }
        }

        // Agenda view itself
        Item {
            id: agenda
            visible: calendar.showAgenda

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 4

            function formatDateWithoutYear(date: date): string {
                // Unfortunately Qt overrides ECMA's Date.toLocaleDateString(),
                // which is able to return locale-specific date-and-month-only date
                // formats, with its dumb version that only supports Qt::DateFormat
                // enum subset. So to get a day-and-month-only date format string we
                // must resort to this magic and hope there are no locales that use
                // other separators...
                const format = Qt.locale().dateFormat(Locale.ShortFormat).replace(/[./ ]*Y{2,4}[./ ]*/i, '');
                return Qt.formatDate(date, format);
            }

            function dateEquals(date1: date, date2: date): bool {
                // QDate is exposed to QML as midnight UTC, while MonthView's
                // current date is constructed in the local time zone.
                return date1.getUTCFullYear() === date2.getFullYear()
                    && date1.getUTCMonth() === date2.getMonth()
                    && date1.getUTCDate() === date2.getDate();
            }

            function updateEventsForCurrentDate() {
                eventsList.model = monthView.daysModel.eventsForDate(monthView.currentDate);
            }

            Connections {
                target: monthView

                function onCurrentDateChanged() {
                    agenda.updateEventsForCurrentDate();
                }
            }

            Connections {
                target: monthView.daysModel

                function onAgendaUpdated(updatedDate: date) {
                    if (agenda.dateEquals(updatedDate, monthView.currentDate)) {
                        agenda.updateEventsForCurrentDate();
                    }
                }
            }

            TextMetrics {
                id: dateLabelMetrics

                // Date/time are arbitrary values with all parts being two-digit
                readonly property string timeString: Qt.formatTime(new Date(2000, 12, 12, 12, 12, 12, 12))
                readonly property string dateString: agenda.formatDateWithoutYear(new Date(2000, 12, 12, 12, 12, 12))

                font: Kirigami.Theme.defaultFont
                text: timeString.length > dateString.length ? timeString : dateString
            }

            PlasmaComponents.ScrollView {
                id: eventsView
                anchors.fill: parent

                ListView {
                    id: eventsList

                    focus: false
                    activeFocusOnTab: true
                    highlight: null
                    currentIndex: -1

                    KeyNavigation.down: switchTimeZoneButton.visible ? switchTimeZoneButton : clocksList
                    Keys.onRightPressed: event => switchTimeZoneButton.Keys.rightPressed(event)

                    onCurrentIndexChanged: if (!activeFocus) {
                        currentIndex = -1;
                    }

                    onActiveFocusChanged: if (activeFocus) {
                        currentIndex = 0;
                    } else {
                        currentIndex = -1;
                    }

                    delegate: PlasmaComponents.ItemDelegate {
                        id: eventItem

                        // Crashes if the type is declared as eventData (which is Q_GADGET)
                        required property /*PlasmaCalendar.eventData*/var modelData

                        width: ListView.view.width
                        topPadding: Kirigami.Units.largeSpacing
                        bottomPadding: Kirigami.Units.largeSpacing

                        leftPadding: calendar.paddings

                        text: eventTitle.text
                        hoverEnabled: true
                        highlighted: ListView.isCurrentItem
                        Accessible.description: modelData.description
                        readonly property bool hasTime: {
                            // Explicitly all-day event
                            if (modelData.isAllDay) {
                                return false;
                            }
                            // Multi-day event which does not start or end today (so
                            // is all-day from today's point of view)
                            if (modelData.startDateTime - monthView.currentDate < 0 &&
                                modelData.endDateTime - monthView.currentDate > 86400000) { // 24hrs in ms
                                return false;
                            }

                            // Non-explicit all-day event
                            const startIsMidnight = modelData.startDateTime.getHours() === 0
                                && modelData.startDateTime.getMinutes() === 0;

                            const endIsMidnight = modelData.endDateTime.getHours() === 0
                                && modelData.endDateTime.getMinutes() === 0;

                            const sameDay = modelData.startDateTime.getDate() === modelData.endDateTime.getDate()
                                && modelData.startDateTime.getDay() === modelData.endDateTime.getDay();

                            return !(startIsMidnight && endIsMidnight && sameDay);
                        }

                        PlasmaComponents.ToolTip {
                            text: eventItem.modelData.description
                            visible: text !== "" && eventItem.hovered
                        }

                        contentItem: GridLayout {
                            id: eventGrid
                            columns: 3
                            rows: 2
                            rowSpacing: 0
                            columnSpacing: Kirigami.Units.largeSpacing

                            Rectangle {
                                id: eventColor

                                Layout.row: 0
                                Layout.column: 0
                                Layout.rowSpan: 2
                                Layout.fillHeight: true

                                color: eventItem.modelData.eventColor
                                implicitWidth: 4
                                radius: 2
                                visible: eventItem.modelData.eventColor !== ""
                            }

                            PlasmaComponents.Label {
                                id: startTimeLabel

                                readonly property bool startsToday: eventItem.modelData.startDateTime - monthView.currentDate >= 0
                                readonly property bool startedYesterdayLessThan12HoursAgo: eventItem.modelData.startDateTime - monthView.currentDate >= -43200000 //12hrs in ms

                                Layout.row: 0
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: startsToday || startedYesterdayLessThan12HoursAgo
                                    ? Qt.formatTime(eventItem.modelData.startDateTime)
                                    : agenda.formatDateWithoutYear(eventItem.modelData.startDateTime)
                                textFormat: Text.PlainText
                                horizontalAlignment: Qt.AlignRight
                                visible: eventItem.hasTime
                            }

                            PlasmaComponents.Label {
                                id: endTimeLabel

                                readonly property bool endsToday: eventItem.modelData.endDateTime - monthView.currentDate <= 86400000 // 24hrs in ms
                                readonly property bool endsTomorrowInLessThan12Hours: eventItem.modelData.endDateTime - monthView.currentDate <= 86400000 + 43200000 // 36hrs in ms

                                Layout.row: 1
                                Layout.column: 1
                                Layout.minimumWidth: dateLabelMetrics.width

                                text: endsToday || endsTomorrowInLessThan12Hours
                                    ? Qt.formatTime(eventItem.modelData.endDateTime)
                                    : agenda.formatDateWithoutYear(eventItem.modelData.endDateTime)
                                textFormat: Text.PlainText
                                horizontalAlignment: Qt.AlignRight
                                opacity: 0.75

                                visible: eventItem.hasTime
                            }

                            PlasmaComponents.Label {
                                id: eventTitle

                                Layout.row: 0
                                Layout.column: 2
                                Layout.fillWidth: true

                                elide: Text.ElideRight
                                text: eventItem.modelData.title
                                textFormat: Text.PlainText
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 2
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: eventsView
                width: eventsView.width - (Kirigami.Units.gridUnit * 8)

                visible: eventsList.count === 0

                iconName: "checkmark"
                text: monthView.isToday(monthView.currentDate)
                    ? i18nd("plasma_applet_org.kde.plasma.digitalclock", "No events for today")
                    : i18nd("plasma_applet_org.kde.plasma.digitalclock", "No events for this day");
            }
        }

        // Horizontal separator line between events and time zones
        KSvg.SvgItem {
            visible: worldClocks.visible && agenda.visible

            Layout.fillWidth: true
            Layout.preferredHeight: naturalSize.height

            imagePath: "widgets/line"
            elementId: "horizontal-line"
        }

        // Clocks stuff
        // ------------
        // Header text + button to change time & time zone
        PlasmaExtras.PlasmoidHeading {
            visible: worldClocks.visible
            background: Item {}

            enabledBorders: Qt.TopEdge | Qt.BottomEdge
            // Normally gets some positive/negative values from base component.
            topInset: 0
            topPadding: Kirigami.Units.smallSpacing

            leftInset: 0
            rightInset: 0
            leftPadding: mirrored ? Kirigami.Units.smallSpacing : calendar.paddings
            rightPadding: mirrored ? calendar.paddings : Kirigami.Units.smallSpacing

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true

                    level: 2

                    text: i18nd("plasma_applet_org.kde.plasma.digitalclock", "Time Zones")
                    textFormat: Text.PlainText
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    Accessible.ignored: true
                }

                PlasmaComponents.ToolButton {
                    id: switchTimeZoneButton

                    visible: KConfig.KAuthorized.authorizeControlModule("kcm_clock.desktop")
                    text: i18nd("plasma_applet_org.kde.plasma.digitalclock", "Switch…")
                    icon.name: "preferences-system-time"

                    Accessible.name: i18nd("plasma_applet_org.kde.plasma.digitalclock", "Switch to another time zone")
                    Accessible.description: i18nd("plasma_applet_org.kde.plasma.digitalclock", "Switch to another time zone")

                    KeyNavigation.down: clocksList
                    Keys.onRightPressed: event => {
                        monthView.Keys.downPressed(event);
                    }

                    onClicked: KCMUtils.KCMLauncher.openSystemSettings("kcm_clock")

                    PlasmaComponents.ToolTip {
                        text: parent.Accessible.description
                    }
                }
            }
        }

        // Clocks view itself
        PlasmaComponents.ScrollView {
            id: worldClocks
            visible: calendar.showClocks

            Layout.fillWidth: true
            Layout.fillHeight: !agenda.visible
            Layout.minimumHeight: visible ? Kirigami.Units.gridUnit * 7 : 0
            Layout.maximumHeight: agenda.visible ? Kirigami.Units.gridUnit * 10 : -1

            ListView {
                id: clocksList
                spacing: Kirigami.Units.smallSpacing

                activeFocusOnTab: true

                highlight: null
                currentIndex: -1
                onActiveFocusChanged: if (activeFocus) {
                    currentIndex = 0;
                } else {
                    currentIndex = -1;
                }

                Keys.onRightPressed: event => {
                    switchTimeZoneButton.Keys.rightPressed(event);
                }

                // Can't use KeyNavigation.tab since the focus won't go to config button, instead it will be redirected to somewhere else because of
                // some existing code. Since now the header was in this file and this was not a problem. Now the header is also implicitly
                // inside the monthViewWrapper.
                Keys.onTabPressed: event => {
                    monthView.viewHeader.configureButton.forceActiveFocus(Qt.BacktabFocusReason);
                }

                model: root.selectedTimeZonesDeduplicatingExplicitLocalTimeZone()

                delegate: PlasmaComponents.ItemDelegate {
                    id: listItem

                    required property string modelData

                    readonly property bool isCurrentTimeZone: tzClock.timeZone == root.currentTimeZone
                    readonly property string tzLabel: {
                        switch (Plasmoid.configuration.displayTimezoneFormat) {
                        case 0: // Code
                            return tzClock.timeZoneCode;
                        case 1: // City
                            return TimeZonesI18n.i18nCity(tzClock.timeZone);
                        case 2: // Offset from UTC time
                            return tzClock.timeZoneOffset;
                        }
                    }

                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin

                    leftPadding: calendar.paddings
                    rightPadding: calendar.paddings

                    topPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.largeSpacing
                    background: Rectangle {
                        radius: 12
                        color: Qt.alpha(Kirigami.Theme.highlightColor, listItem.highlighted ? 0.18 : listItem.isCurrentTimeZone ? 0.09 : 0.035)
                        border.color: listItem.activeFocus ? Kirigami.Theme.highlightColor : "transparent"
                    }
                    highlighted: ListView.isCurrentItem
                    Accessible.name: tzLabel
                    Accessible.description: root.formatTime(tzClock.dateTime, Plasmoid.configuration.showSeconds === 2)

                    // Only highlight with keyboard
                    down: false
                    hoverEnabled: false

                    Clock {
                        id: tzClock
                        timeZone: listItem.modelData
                        trackSeconds: Plasmoid.configuration.showSeconds === 2 // Always
                    }

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            PlasmaComponents.Label {
                                Layout.fillWidth: true
                                text: listItem.tzLabel
                                textFormat: Text.PlainText
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            PlasmaComponents.Label {
                                Layout.fillWidth: true
                                visible: !listItem.isCurrentTimeZone
                                text: root.formatOffset(tzClock.dateTime).replace(/^\s*•\s*/, "")
                                textFormat: Text.PlainText
                                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
                                color: Kirigami.Theme.disabledTextColor
                                elide: Text.ElideRight
                            }
                        }
                        PlasmaComponents.Label {
                            Layout.maximumWidth: listItem.width * 0.55
                            text: root.formatTime(tzClock.dateTime, Plasmoid.configuration.showSeconds === 2)
                            textFormat: Text.PlainText
                            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.4
                            font.weight: Font.Medium
                            font.features: { "tnum": 1 }
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    // Trailing column containing calendar
    // ===============================
    FocusScope {
        id: monthViewWrapper

        // Inherit the live desktop palette so dates and controls follow
        // light/dark mode and accent changes along with the card surface.

        anchors {
            top: clockHeader.bottom
            topMargin: calendar.paddings
            right: parent.right
            rightMargin: calendar.paddings
            bottom: parent.bottom
            bottomMargin: calendar.paddings
        }

        // Not anchoring to horizontalCenter to avoid sub-pixel misalignments
        width: (calendar.showAgenda || calendar.showClocks) ? Math.round(parent.width / 2) - calendar.paddings : parent.width - calendar.paddings * 2

        onActiveFocusChanged: if (activeFocus) {
            monthViewWrapper.nextItemInFocusChain().forceActiveFocus();
        }

        SequoiaCalendar.MonthView {
            id: monthView
            objectName: "sequoiaMonthView"
            viewHeader.height: calendar.headerHeight

            anchors {
                fill: parent
                margins: calendar.paddings
            }

            borderOpacity: 0

            eventPluginsManager: eventPluginsManager
            today: root.currentTime
            firstDayOfWeek: Plasmoid.configuration.firstDayOfWeek > -1
                ? Plasmoid.configuration.firstDayOfWeek
                : Qt.locale().firstDayOfWeek
            showWeekNumbers: Plasmoid.configuration.showWeekNumbers

            showDigitalClockHeader: true
            digitalClock: Plasmoid
            eventButton: addEventButton

            KeyNavigation.left: KeyNavigation.tab
            KeyNavigation.tab: addEventButton.visible ? addEventButton : addEventButton.KeyNavigation.down
            Keys.onUpPressed: event => {
                viewHeader.tabBar.currentItem.forceActiveFocus(Qt.BacktabFocusReason);
            }
        }
    }
}
