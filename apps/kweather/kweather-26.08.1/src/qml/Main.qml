/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2021 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import org.kde.kirigami as Kirigami
import org.kde.config as KConfig

import org.kde.kweather.locations
import org.kde.kweather.settings
import org.kde.kweather.components

import org.kde.kweather

Kirigami.AbstractApplicationWindow {
    id: appwindow
    title: i18n("Weather")

    minimumWidth: Kirigami.Settings.isMobile ? 0 : 390
    minimumHeight: Kirigami.Settings.isMobile ? 0 : 480
    width: Kirigami.Settings.isMobile ? Kirigami.Units.gridUnit * 27 : 1180
    height: Kirigami.Settings.isMobile ? Kirigami.Units.gridUnit * 45 : 820
    objectName: "sequoiaWeatherWindow"
    SystemPalette { id: systemPalette }
    readonly property bool atmospheric: settingsModel.forecastStyle === "Dynamic"
    color: atmospheric ? "#15283d" : systemPalette.window.a < 1 ? "transparent" : systemPalette.window
    background: Item {
        layer.enabled: appwindow.locationSearchOpen && GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            blurEnabled: true
            blurMax: 32; blur: 1
            autoPaddingEnabled: false
        }
        Rectangle {
            anchors.fill: parent
            visible: !appwindow.atmospheric
            color: systemPalette.window
        }
        WeatherScene {
            anchors.fill: parent
            opacity: appwindow.atmospheric ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Kirigami.Units.longDuration } }
            condition: appwindow.pageStack.currentItem?.location?.currentHourForecast?.weatherIcon || "weather-few-clouds"
            motion: settingsModel.animateWeather
        }
    }

    pageStack: PageStack {
        // Blur only the app beneath the modal; the dialog and its text stay sharp.
        layer.enabled: appwindow.locationSearchOpen && GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect {
            blurEnabled: true
            blurMax: 32; blur: 1
            autoPaddingEnabled: false
        }
    }

    readonly property bool locationSearchOpen: addLocationDialogLoader.isOpen

    readonly property bool isWideScreen: width > 540

    readonly property bool isDialogOpen: settingsDialogLoader.isOpen || addLocationDialogLoader.isOpen || locationsListDialogLoader.isOpen

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    Component.onCompleted: {
        // initial page
        switchToPage(getPage("Forecast"), 1);
        if (settingsModel.firstStartup) {
            // The empty-state welcome page is the setup entry point. Appearance
            // and units remain available from its Settings button.
            settingsModel.firstStartup = false;
            settingsModel.save();
        }
    }

    Kirigami.PagePool {
        id: pagePool
    }

    function switchToPage(page, depth) {
        while (pageStack.currentItem !== page && pageStack.depth > depth) {
            if (pageStack.depth === 1) {
                pageStack.clear();
            } else {
                pageStack.pop();
            }
        }

        if (pageStack.currentItem !== page) {
            pageStack.push(page);
        }
    }

    function pushPage(page, depth) {
        while (depth !== undefined && pageStack.depth > depth + 1)
            pageStack.pop();
        pageStack.push(page);
    }

    function getPage(name) {
        switch (name) {
        case "Forecast":
            return pagePool.loadPage(WeatherLocationListModel.count === 0 ? getPageUrl('', 'DefaultPage.qml') : getPageUrl('', 'ForecastContainerPage.qml'));
        case "Locations":
            return pagePool.loadPage("qrc:/qt/qml/org/kde/kweather/locations/qml/locationslist/LocationsListPage.qml");
        case "Settings":
            return pagePool.loadPage(getPageUrl('settings', 'settings/SettingsPage.qml'));
        case "Default":
            return pagePool.loadPage(getPageUrl('', 'DefaultPage.qml'));
        }
    }

    function getPageUrl(module: string, file: string): string {
        if (module === '') {
            return `qrc:/qt/qml/org/kde/kweather/qml/${file}`;
        }
        return `qrc:/qt/qml/org/kde/kweather/${module}/qml/${file}`;
    }

    function openSettings() {
        if (isWideScreen) {
            settingsDialogLoader.active = true;
            settingsDialogLoader.item.open();
        } else {
            applicationWindow().pushPage(getPage("Settings"), 0);
        }
    }

    function openLocationsList() {
        if (isWideScreen) {
            locationsListDialogLoader.active = true;
            locationsListDialogLoader.item.open();
        } else {
            applicationWindow().pushPage(getPage("Locations"), 0);
        }
    }

    function openAddLocation() {
        if (isWideScreen) {
            addLocationDialogLoader.active = true;
            addLocationDialogLoader.item.open();
        } else {
            applicationWindow().pageStack.push(getPageUrl('locations', 'locationslist/AddLocationPage.qml'));
        }
    }

    Connections {
        target: WeatherLocationListModel

        function onLocationsChanged() {
            // wwitch to default page if the count is zero
            if (WeatherLocationListModel.count === 0) {
                applicationWindow().switchToPage(applicationWindow().getPage("Default"), 0);
            }
        }
    }

    Loader {
        id: settingsDialogLoader
        property bool isOpen: item && item.visible
        active: false
        sourceComponent: Qt.createComponent("org.kde.kweather.settings", "SettingsDialog")
    }

    Loader {
        id: addLocationDialogLoader
        property bool isOpen: item && item.visible
        active: false
        sourceComponent: AddLocationDialog {}
    }

    Loader {
        id: locationsListDialogLoader
        property bool isOpen: item && item.visible
        active: false
        sourceComponent: LocationsListDialog {}
    }

    FontLoader {
        id: lightHeadingFont
        source: "qrc:/resources/NotoSans-Light.ttf"
    }
}
