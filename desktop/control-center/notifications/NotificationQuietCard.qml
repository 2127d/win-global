// SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// Uses KDE's existing duration choices, settings, and inhibition explanations.
import QtQuick
import QtQuick.Layouts
import QtQml.Models
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import org.kde.coreaddons as KCoreAddons
import "../details" as Details

Details.DetailSurface {
    id: quietCard
    objectName: "notificationQuietCard"
    required property var notificationSettings
    required property var inhibition
    required property bool serverAvailable
    property Item nextFocusItem
    readonly property int dndMorningHour: 6
    readonly property int dndEveningHour: 20
    readonly property alias toggle: dndCheck
    readonly property string reason: {
                    if (!quietCard.inhibition.inhibited) {
                        return "";
                    }

                    var inhibitedUntil = quietCard.notificationSettings.notificationsInhibitedUntil;
                    var inhibitedUntilTime = inhibitedUntil.getTime();
                    var inhibitedByApp = quietCard.notificationSettings.notificationsInhibitedByApplication;
                    var inhibitedByMirroredScreens = quietCard.notificationSettings.inhibitNotificationsWhenScreensMirrored
                                                        && quietCard.notificationSettings.screensMirrored;
                    var inhibitedByFullscreen = quietCard.notificationSettings.inhibitNotificationsWhenFullscreen
                                                        && quietCard.notificationSettings.fullscreenFocused;
                    var dateNow = Date.now();

                    var sections = [];

                    // Show until time if valid but not if too far int he future
                    if (!isNaN(inhibitedUntilTime) && inhibitedUntilTime - dateNow > 0 &&
                        inhibitedUntilTime - dateNow < 100 * 24 * 60 * 60 * 1000 /* 1 year*/) {
                        const endTime = KCoreAddons.Format.formatRelativeDateTime(inhibitedUntil, Locale.ShortFormat);
                        const lowercaseEndTime = endTime[0] + endTime.slice(1);
                        sections.push(i18nc("Do not disturb until date", "Automatically ends: %1", lowercaseEndTime));
                    }

                    if (inhibitedByApp) {
                        var inhibitionAppNames = quietCard.notificationSettings.notificationInhibitionApplications;
                        var inhibitionAppReasons = quietCard.notificationSettings.notificationInhibitionReasons;

                        for (var i = 0, length = inhibitionAppNames.length; i < length; ++i) {
                            var name = inhibitionAppNames[i];
                            var reason = inhibitionAppReasons[i];

                            if (reason) {
                                sections.push(i18nc("Do not disturb until app has finished (reason)", "While %1 is active (%2)", name, reason));
                            } else {
                                sections.push(i18nc("Do not disturb until app has finished", "While %1 is active", name));
                            }
                        }
                    }

                    if (inhibitedByMirroredScreens) {
                        sections.push(i18nc("Do not disturb because external mirrored screens connected", "Screens are mirrored"))
                    }

                    if (inhibitedByFullscreen) {
                        sections.push(i18nc("Do not disturb because fullscreen app is focused", "Fullscreen app is focused"))
                    }

                    return sections.join(" · ");
    }
    implicitHeight: content.implicitHeight + 28
    radius: 24
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 14; spacing: 10
        RowLayout {
            spacing: 12
            Rectangle {
                Layout.preferredWidth: 40; Layout.preferredHeight: 40; radius: 14
                color: Qt.alpha(Kirigami.Theme.highlightColor, quietCard.inhibition.inhibited ? 0.22 : 0.10)
                Kirigami.Icon { anchors.centerIn: parent; width: 22; height: 22; source: "notifications-disabled"; isMask: true; color: Kirigami.Theme.textColor }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                PC.Label { Layout.fillWidth: true; text: qsTr("Do Not Disturb"); font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                PC.Label { Layout.fillWidth: true; text: quietCard.inhibition.inhibited ? qsTr("On") : qsTr("Off"); color: Kirigami.Theme.disabledTextColor }
            }
            PC.Switch {
                id: dndCheck
                objectName: "notificationDndToggle"
                enabled: quietCard.serverAvailable
                checked: quietCard.inhibition.inhibited
                Accessible.name: qsTr("Do Not Disturb")
                Accessible.description: quietCard.reason || qsTr("Choose how long to pause notification banners")
                KeyNavigation.down: quietCard.nextFocusItem
                KeyNavigation.tab: quietCard.nextFocusItem
                onToggled: checked = Qt.binding(() => quietCard.inhibition.inhibited)
                onPressed: {
                    if (!quietCard.inhibition.inhibited) {
                        dndMenu.date = new Date();
                        dndMenu.popup(dndCheck, 0, dndCheck.height);
                    }
                }
                onClicked: if (quietCard.inhibition.inhibited) quietCard.inhibition.revokeInhibitions()
                Accessible.onPressAction: {
                    if (quietCard.inhibition.inhibited) quietCard.inhibition.revokeInhibitions();
                    else { dndMenu.date = new Date(); dndMenu.popup(dndCheck, 0, dndCheck.height); }
                }
            }
        }
        PC.Label {
            objectName: "notificationInhibitionReason"
            Layout.fillWidth: true
            text: quietCard.reason || (quietCard.inhibition.inhibited ? qsTr("Notification banners are paused.") : qsTr("Choose how long to pause notification banners."))
            wrapMode: Text.Wrap; textFormat: Text.PlainText; color: Kirigami.Theme.disabledTextColor
        }
    }
    NotificationDurationMenu {
        id: dndMenu
        objectName: "notificationDurationMenu"
        property date date: new Date()
        readonly property var choices: {
                            var model = [];

                            // For 1 hour
                            var d = dndMenu.date;
                            d.setHours(d.getHours() + 1);
                            d.setSeconds(0);
                            model.push({date: d, text: i18n("For 1 hour")});

                            d = dndMenu.date;
                            d.setHours(d.getHours() + 4);
                            d.setSeconds(0);
                            model.push({date: d, text: i18n("For 4 hours")});

                            // Until this evening
                            if (dndMenu.date.getHours() < quietCard.dndEveningHour) {
                                d = dndMenu.date;
                                // TODO make the user's preferred time schedule configurable
                                d.setHours(quietCard.dndEveningHour);
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until this evening")});
                            }

                            // Until next morning
                            if (dndMenu.date.getHours() > quietCard.dndMorningHour) {
                                d = dndMenu.date;
                                d.setDate(d.getDate() + 1);
                                d.setHours(quietCard.dndMorningHour);
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until tomorrow morning")});
                            }

                            // Until Monday
                            // show Friday and Saturday, Sunday is "0" but for that you can use "until tomorrow morning"
                            if (dndMenu.date.getDay() >= 5) {
                                d = dndMenu.date;
                                d.setHours(quietCard.dndMorningHour);
                                // wraps around if necessary
                                d.setDate(d.getDate() + (7 - d.getDay() + 1));
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until Monday")});
                            }

                            // Until "turned off"
                            d = dndMenu.date;
                            // Just set it to one year in the future so we don't need yet another "do not disturb enabled" property
                            d.setFullYear(d.getFullYear() + 1);
                            model.push({date: d, text: i18n("Until manually disabled")});

                            return model;
        }
        Instantiator {
            model: dndMenu.choices
            delegate: NotificationDurationMenuItem {
                required property var modelData
                text: modelData.text
                objectName: "notificationDuration:" + index
                required property int index
                onTriggered: {
                    quietCard.notificationSettings.notificationsInhibitedUntil = modelData.date;
                    quietCard.notificationSettings.save();
                }
            }
            onObjectAdded: (index, object) => dndMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => dndMenu.removeItem(object)
        }
    }
}
