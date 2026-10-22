/*
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013-2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.ksvg as KSvg
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

import org.kde.plasma.clock
import org.kde.plasma.workspace.calendar as PlasmaCalendar

PlasmoidItem {
    id: analogclock
    objectName: "sequoiaAnalogClock"
    readonly property bool panel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    Layout.minimumWidth: panel ? Kirigami.Units.iconSizes.smallMedium : 96
    Layout.minimumHeight: panel ? Kirigami.Units.iconSizes.smallMedium : 96
    Layout.preferredWidth: panel ? 32 : 192
    Layout.preferredHeight: panel ? 32 : 192

    width: Kirigami.Units.gridUnit * 15
    height: Kirigami.Units.gridUnit * 15

    readonly property string currentTime: Qt.locale().toString(clockSource.dateTime, Qt.locale().timeFormat(Locale.LongFormat))
    readonly property string currentDate: Qt.locale().toString(clockSource.dateTime, Qt.locale().dateFormat(Locale.LongFormat).replace(/(^dddd.?\s)|(,?\sdddd$)/, ""))

    property int hours: clockSource.dateTime.getHours()
    property int minutes: clockSource.dateTime.getMinutes()
    property int seconds: clockSource.dateTime.getSeconds()
    property bool showSecondsHand: Plasmoid.configuration.showSecondHand
    property bool showTimezone: Plasmoid.configuration.showTimezoneString

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    preferredRepresentation: compactRepresentation

    toolTipMainText: {
        const day = Qt.locale().toString(clockSource.dateTime, "dddd");
        return day.charAt(0).toUpperCase() + day.slice(1);
    }
    toolTipSubText: `${currentTime}\n${currentDate}`

    Clock {
        id: clockSource
        trackSeconds: analogclock.showSecondsHand || !!analogclock.compactRepresentationItem?.containsMouse
    }

    compactRepresentation: MouseArea {
        id: representation

        Layout.minimumWidth: analogclock.panel ? Kirigami.Units.iconSizes.smallMedium : 96
        Layout.minimumHeight: analogclock.panel ? Kirigami.Units.iconSizes.smallMedium : 96

        property bool wasExpanded

        activeFocusOnTab: true
        hoverEnabled: true

        Accessible.name: Plasmoid.title
        Accessible.description: i18nc("@info:tooltip", "Current time is %1; Current date is %2", analogclock.currentTime, analogclock.currentDate)
        Accessible.role: Accessible.Button

        Accessible.onPressAction: analogclock.expanded = !analogclock.expanded
        Keys.onSpacePressed: analogclock.expanded = !analogclock.expanded
        Keys.onReturnPressed: analogclock.expanded = !analogclock.expanded
        onPressed: wasExpanded = analogclock.expanded
        onClicked: analogclock.expanded = !wasExpanded

        ClockFace {
            anchors {
                top: parent.top
                bottom: analogclock.showTimezone ? timezoneBg.top : parent.bottom
                left: parent.left
                right: parent.right
                margins: analogclock.panel ? 0 : 6
            }
            rounded: Plasmoid.configuration.faceStyle !== "original"
            hours: analogclock.hours
            minutes: analogclock.minutes
            seconds: analogclock.seconds
            showSecondsHand: analogclock.showSecondsHand
        }

        KSvg.FrameSvgItem {
            id: timezoneBg

            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: 10
            }
            width: Math.min(parent.width - 12, timezoneText.implicitWidth + margins.right + margins.left)
            height: timezoneText.implicitHeight + margins.top + margins.bottom
            visible: analogclock.showTimezone

            imagePath: "widgets/background"

            PlasmaComponents.Label {
                id: timezoneText
                x: timezoneBg.margins.left
                y: timezoneBg.margins.top
                width: Math.max(0, timezoneBg.width - timezoneBg.margins.left - timezoneBg.margins.right)
                text: clockSource.timeZone
                textFormat: Text.PlainText
                elide: Text.ElideRight
            }
        }
    }

    fullRepresentation: PlasmaCalendar.MonthView {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 22
        Layout.maximumWidth: Kirigami.Units.gridUnit * 80
        Layout.minimumHeight: Kirigami.Units.gridUnit * 22
        Layout.maximumHeight: Kirigami.Units.gridUnit * 40

        readonly property var appletInterface: analogclock

        today: clockSource.dateTime
    }
}
