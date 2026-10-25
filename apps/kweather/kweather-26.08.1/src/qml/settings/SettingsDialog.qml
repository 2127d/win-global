// SPDX-FileCopyrightText: 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kweather
import org.kde.kweather.components

// The StackView owns scrolling, so this shell needs no additional scroll container.
Controls.Dialog {
    id: root
    title: pages.depth > 1 ? i18n("About Weather") : i18n("Settings")
    objectName: "weatherSettingsDialog"
    standardButtons: Controls.Dialog.NoButton
    property bool showCloseButton: true
    modal: true
    padding: 0
    spacing: 0
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnReleaseOutside
    // Keep the dialog above the app's backdrop layers so only the forecast blurs.
    popupType: Controls.Popup.Item
    parent: applicationWindow().overlay

    readonly property SystemPalette themePalette: SystemPalette { id: systemPalette }
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2) + Kirigami.Units.gridUnit * 2 * (1 - opacity)
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
    }
    // Main owns the single visual scrim; Popup still handles modal input.
    Controls.Overlay.modal: Item {}
    background: Kirigami.ShadowedRectangle {
        radius: 24
        color: systemPalette.window
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.13)
        shadow.size: Kirigami.Units.largeSpacing
        shadow.color: Qt.alpha(Kirigami.Theme.textColor, 0.10)
    }
    header: Controls.Control {
        padding: 12
        background: null
        contentItem: Item {
            implicitHeight: 32
            Controls.Label {
                objectName: "weatherSettingsDialogTitle"
                anchors.fill: parent
                anchors.leftMargin: 44; anchors.rightMargin: 44
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 18
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            TrafficLightClose {
                objectName: "weatherCloseSettings"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                visible: root.showCloseButton && pages.depth === 1
                onClicked: root.reject()
            }
            WeatherAction {
                id: backButton
                objectName: "weatherAboutBack"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 38; height: 38
                topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
                visible: pages.depth > 1
                enabled: !pages.busy
                floating: true
                iconOnly: true
                text: i18n("Back to Settings")
                symbol: root.mirrored ? "go-next-symbolic" : "go-previous-symbolic"
                onClicked: pages.pop()
            }
        }
    }
    footer: Item { implicitHeight: 0 }

    readonly property real preferredWidth: Kirigami.Units.gridUnit * 35
    implicitHeight: contentItem.implicitHeight + header.implicitHeight
    width: Math.min(preferredWidth, parent.width - 48)
    height: Math.min(implicitHeight, parent.height - 48)

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    onClosed: {
        if (pages.depth > 1)
            pages.pop(null, Controls.StackView.Immediate);
    }

    contentItem: Controls.StackView {
        id: pages
        objectName: "weatherSettingsPages"
        implicitWidth: root.preferredWidth
        // Keep the popup's footprint stable while navigating to longer About content.
        implicitHeight: settingsContent.collapsedHeight + 2 * Kirigami.Units.gridUnit
        clip: true
        initialItem: Basic.ScrollView {
            id: settingsScroll
            implicitWidth: root.preferredWidth
            implicitHeight: settingsContent.collapsedHeight + 2 * Kirigami.Units.gridUnit
            contentWidth: availableWidth
            topPadding: Kirigami.Units.gridUnit
            bottomPadding: Kirigami.Units.gridUnit
            clip: true
            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
            SettingsComponent {
                id: settingsContent
                viewport: settingsScroll.contentItem
                dialog: root
                width: settingsScroll.availableWidth
                onCloseRequested: root.close()
                onAboutRequested: {
                    if (!pages.busy && pages.depth === 1)
                        pages.push(aboutView);
                }
            }
        }
        Component { id: aboutView; AboutView {} }
        onBusyChanged: {
            if (!busy) {
                if (depth > 1)
                    backButton.forceActiveFocus();
                else
                    settingsContent.focusAbout();
            }
        }
        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: root.mirrored ? -24 : 24; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
        pushExit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: 0; to: root.mirrored ? 24 : -24; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
        popEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: root.mirrored ? 24 : -24; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
        popExit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: 0; to: root.mirrored ? -24 : 24; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
    }
}
