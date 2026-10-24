// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as Dialogs

Kirigami.Dialog {
    id: root
    property string closeButtonObjectName: "weatherCloseLocations"
    standardButtons: Kirigami.Dialog.NoButton
    parent: applicationWindow().overlay
    preferredHeight: 440
    preferredWidth: 480
    padding: 16
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    background: LocationSurface {
        radius: 24
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.88)
    }
    header: Dialogs.DialogHeader {
        dialog: root
        background: null
        leftPadding: padding + 2
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Label {
                objectName: "weatherLocationDialogTitle"
                Layout.fillWidth: true
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 18
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            ToolButton {
                id: closeButton
                objectName: root.closeButtonObjectName
                Layout.preferredWidth: 32; Layout.preferredHeight: 32
                padding: 8
                text: i18n("Close")
                Accessible.name: text
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
                ToolTip.visible: hovered
                ToolTip.text: text
                ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }
    }

}
