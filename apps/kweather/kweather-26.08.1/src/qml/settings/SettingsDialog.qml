// SPDX-FileCopyrightText: 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as Dialogs

Kirigami.Dialog {
    id: root
    title: i18n("Settings")
    standardButtons: Kirigami.Dialog.NoButton
    // Keep the dialog above the app's backdrop layers so only the forecast blurs.
    popupType: Kirigami.Dialog.Item
    parent: applicationWindow().overlay

    SystemPalette { id: systemPalette }
    background: Kirigami.ShadowedRectangle {
        radius: 24
        color: systemPalette.window
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.13)
        shadow.size: Kirigami.Units.largeSpacing
        shadow.color: Qt.alpha(Kirigami.Theme.textColor, 0.10)
    }
    header: Dialogs.DialogHeader {
        dialog: root
        background: null
        leftPadding: padding + 2
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Controls.Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 18
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            Controls.ToolButton {
                id: closeButton
                objectName: "weatherCloseSettings"
                Layout.preferredWidth: 32; Layout.preferredHeight: 32
                padding: 8
                text: i18n("Close")
                Accessible.name: text
                visible: root.showCloseButton
                onClicked: root.reject()
                contentItem: Kirigami.Icon {
                    source: "window-close-symbolic"
                    isMask: true
                    color: Kirigami.Theme.textColor
                }
                background: Rectangle {
                    radius: width / 2
                    color: Qt.alpha(Kirigami.Theme.textColor, closeButton.down ? 0.16 : closeButton.hovered ? 0.10 : 0.05)
                    border.width: closeButton.visualFocus ? 2 : 1
                    border.color: closeButton.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.18)
                    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                }
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: text
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }
    footer: Item { implicitHeight: 0 }

    preferredWidth: Kirigami.Units.gridUnit * 35

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    Controls.Control {
        id: control
        leftPadding: 0
        rightPadding: 0
        topPadding: Kirigami.Units.gridUnit
        bottomPadding: Kirigami.Units.gridUnit

        contentItem: SettingsComponent {
            dialog: root
            width: control.width

            onCloseRequested: root.close();
        }
    }
}
