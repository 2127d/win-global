/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020 Devin Lin <espidev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kweather

// The first-launch screen also returns when the last saved location is removed.
Kirigami.Page {
    id: root
    objectName: "sequoiaWeatherWelcome"
    padding: 0
    titleDelegate: null
    background: Item {}
    readonly property bool atmospheric: settingsModel.forecastStyle === "Dynamic"
    readonly property color ink: atmospheric ? "#f1f6fc" : Kirigami.Theme.textColor
    readonly property color muted: atmospheric ? "#c9dce9" : Kirigami.Theme.disabledTextColor

    WeatherAction {
        objectName: "weatherWelcomeSettings"
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 20
        text: i18n("Settings"); symbol: "settings-configure"; iconOnly: true
        atmospheric: root.atmospheric
        onClicked: applicationWindow().openSettings()
    }

    ColumnLayout {
        id: welcome
        anchors.centerIn: parent
        width: Math.min(440, parent.width - 48)
        spacing: 0

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 120; Layout.preferredHeight: 120
            Layout.bottomMargin: 32
            radius: 30
            gradient: Gradient {
                GradientStop { position: 0; color: "#42b8f2" }
                GradientStop { position: 1; color: "#1762c4" }
            }
            border.width: 1
            border.color: "#80d8faff"
            WeatherGlyph {
                anchors.centerIn: parent
                width: 112; height: 100
                anchors.verticalCenterOffset: 7
                condition: "weather-few-clouds"
            }
        }
        Label {
            Layout.fillWidth: true
            text: i18n("Welcome to Weather")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: root.width < 500 ? 28 : 32
            font.weight: Font.DemiBold
            color: root.ink
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: 12; Layout.bottomMargin: 32
            text: i18n("Add a location to see the weather ahead.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 15
            color: root.muted
        }
        Button {
            id: addLocation
            objectName: "weatherWelcomeAddLocation"
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Math.max(180, contentItem.implicitWidth + 48)
            implicitHeight: 48
            leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
            padding: 12
            text: i18n("Add location")
            Accessible.name: text
            hoverEnabled: true
            onClicked: applicationWindow().openAddLocation()
            background: Rectangle {
                radius: 16
                color: addLocation.down ? Qt.darker(Kirigami.Theme.highlightColor,1.18)
                    : addLocation.hovered ? Qt.lighter(Kirigami.Theme.highlightColor,1.12)
                    : Kirigami.Theme.highlightColor
                border.width: addLocation.visualFocus ? 2 : 0
                border.color: root.ink
                Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
            }
            contentItem: Label {
                text: addLocation.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14; font.weight: Font.DemiBold
                color: Kirigami.Theme.highlightedTextColor
            }
        }
    }
}
