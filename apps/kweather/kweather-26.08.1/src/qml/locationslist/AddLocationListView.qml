/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2022 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import org.kde.kweather

import org.kde.kweather.components

Item {
    id: root
    objectName: "weatherLocationSearch"
    implicitWidth: 480
    implicitHeight: 360
    property string searchQuery: ""
    readonly property bool editingSearch: search.activeFocus
    property alias model: results.model
    property alias currentIndex: results.currentIndex
    readonly property int count: results.count
    signal closeRequested
    signal focusRequested
    function positionViewAtEnd() {
        results.positionViewAtEnd();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Control {
            id: searchHeader
            Layout.fillWidth: true
            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.SearchField {
                    id: search
                    objectName: "weatherLocationSearchField"
                    Layout.fillWidth: true
                    implicitHeight: 44
                    rightActions: []
                    leftPadding: LayoutMirroring.enabled ? 44 : 28
                    rightPadding: LayoutMirroring.enabled ? 28 : 44

                    ToolButton {
                        id: clearButton
                        objectName: "weatherClearLocationSearch"
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 28
                        padding: 6
                        leftInset: 0
                        rightInset: 0
                        topInset: 0
                        bottomInset: 0
                        visible: search.text.length > 0
                        hoverEnabled: true
                        focusPolicy: Qt.TabFocus
                        text: i18n("Clear search")
                        Accessible.name: text
                        onClicked: {
                            search.clear();
                            search.accepted();
                            search.forceActiveFocus();
                        }
                        contentItem: Kirigami.Icon {
                            source: search.LayoutMirroring.enabled ? "edit-clear-locationbar-ltr" : "edit-clear-locationbar-rtl"
                            isMask: true
                            color: Kirigami.Theme.textColor
                        }
                        background: Rectangle {
                            radius: width / 2
                            color: Qt.alpha(Kirigami.Theme.highlightColor, clearButton.down ? 0.24 : clearButton.hovered || clearButton.activeFocus ? 0.14 : 0)
                            border.width: clearButton.visualFocus ? 2 : 0
                            border.color: Kirigami.Theme.highlightColor
                            Behavior on color {
                                ColorAnimation {
                                    duration: Kirigami.Units.shortDuration
                                }
                            }
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: text
                        ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                    background: Rectangle {
                        radius: 22
                        color: Kirigami.Theme.backgroundColor
                        border.width: search.activeFocus ? 2 : 1
                        border.color: search.activeFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.15)
                    }

                    Connections {
                        target: root
                        function onFocusRequested() {
                            search.forceActiveFocus();
                        }
                    }

                    placeholderText: i18n("Search for a location…")
                    onTextChanged: {
                        root.searchQuery = text;
                        root.model.textChanged(text);
                        root.currentIndex = 0;
                    }
                    onEditingFinished: {
                        if (root.searchQuery === text) {
                            // no change
                            return;
                        }
                        root.searchQuery = text;
                        root.model.textChanged(text, 0); // when return is pressed, query immediately
                        root.currentIndex = 0;
                    }
                    KeyNavigation.down: results
                }

                Button {
                    id: searchButton
                    objectName: "weatherLocationSearchButton"
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    padding: 12
                    icon.name: "search"
                    implicitHeight: search.implicitHeight
                    implicitWidth: height
                    enabled: search.text !== ""
                    width: height
                    height: search.height
                    onClicked: root.model.textChanged(root.searchQuery, 0)
                    contentItem: Kirigami.Icon {
                        source: "search"
                        isMask: true
                        implicitWidth: 20
                        implicitHeight: 20
                        opacity: searchButton.enabled ? 1 : 0.45
                    }
                    background: Rectangle {
                        radius: 22
                        color: Qt.alpha(Kirigami.Theme.highlightColor, searchButton.down ? 0.24 : searchButton.hovered ? 0.18 : 0.10)
                        border.width: searchButton.visualFocus ? 2 : 0
                        border.color: Kirigami.Theme.highlightColor
                        opacity: searchButton.enabled ? 1 : 0.45
                        Behavior on color {
                            ColorAnimation {
                                duration: Kirigami.Units.shortDuration
                            }
                        }
                    }
                }
            }
        }

        LocationSurface {
            id: resultCard
            objectName: "weatherSearchResultsCard"
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: searchHeader.leftPadding
            Layout.preferredWidth: searchHeader.availableWidth
            Layout.fillHeight: true
            Layout.bottomMargin: 4
            visible: results.count > 0
            radius: 22
            ListView {
                id: results
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                model: LocationQueryModel {}
                spacing: 0
                ScrollBar.vertical: ScrollBar {
                    id: resultsScrollBar
                    objectName: "weatherSearchResultsScrollBar"
                    policy: ScrollBar.AsNeeded
                }
                delegate: ListDelegate {
                    id: resultRow
                    objectName: "weatherLocationResult" + index
                    // Reserve a separate lane even when an overlay scrollbar fades out.
                    readonly property real scrollBarSpace: resultsScrollBar.size < 1 ? resultsScrollBar.width + 6 : 0
                    x: results.LayoutMirroring.enabled ? scrollBarSpace : 0
                    width: Math.max(0, results.width - scrollBarSpace)
                    background: Rectangle {
                        radius: 12
                        color: resultRow.mouseArea.pressed ? Qt.alpha(Kirigami.Theme.highlightColor, 0.16) : !root.editingSearch && (resultRow.activeFocus || (resultRow.ListView.isCurrentItem && results.activeFocus)) ? Qt.alpha(Kirigami.Theme.highlightColor, 0.10) : resultRow.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.05) : "transparent"
                        Behavior on color {
                            ColorAnimation {
                                duration: Kirigami.Units.shortDuration
                            }
                        }
                    }

                    leftPadding: Kirigami.Units.largeSpacing
                    rightPadding: Kirigami.Units.largeSpacing
                    topPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing * 2
                    bottomPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing * 2

                    function apply() {
                        root.model.addLocation(index);
                        applicationWindow().getPage("Forecast").switchPageIndex(WeatherLocationListModel.count - 1);
                        switchToPage(appwindow.getPage("Forecast"), 0);
                        root.closeRequested();
                    }

                    onClicked: apply()
                    Keys.onPressed: event => {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            apply();
                            event.accepted = true;
                        } else {
                            event.accepted = false;
                        }
                    }

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        Label {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            text: model.name
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: results.count === 0
            // unable to connect message
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.largeSpacing

                icon.name: "network-disconnect"
                text: i18n("Unable to connect")
                visible: root.model.networkError
            }

            // default message (has not searched yet)
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.largeSpacing

                icon.name: "search"
                text: i18n("Search for a location")
                visible: !root.model.networkError && !root.model.loading && root.count == 0 && root.searchQuery == ""
            }

            // no results
            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.largeSpacing

                icon.name: "search"
                text: i18n("No results")
                visible: !root.model.networkError && !root.model.loading && root.count == 0 && root.searchQuery != ""
            }

            // loading results indicator
            BusyIndicator {
                anchors.centerIn: parent
                running: root.model.loading && root.count === 0
                Layout.minimumWidth: Kirigami.Units.iconSizes.huge
                Layout.minimumHeight: width
            }
        }
    }
}
