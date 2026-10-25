// Copyright 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import "qrc:/sequoia-weather/ActionIcons.js" as ActionIcons
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

Control {
    id: root
    property string title
    property list<T.Action> actions
    property bool showBackButton: false
    Layout.fillWidth: true
    implicitHeight: 56
    leftPadding: 14
    rightPadding: 14
    topPadding: 12
    bottomPadding: 12
    background: null

    contentItem: RowLayout {
        spacing: 12
        ToolButton {
            id: backButton
            objectName: "weatherCompactBack"
            visible: root.showBackButton
            enabled: !applicationWindow().pageStack.busy
            Layout.preferredWidth: 32; Layout.preferredHeight: 32
            padding: 8
            text: i18n("Navigate Back")
            Accessible.name: text
            onClicked: applicationWindow().pageStack.pop()
            contentItem: Kirigami.Icon {
                source: root.mirrored ? ActionIcons.source('go-next-symbolic') : ActionIcons.source('go-previous-symbolic')
                isMask: true
            }
            background: Rectangle {
                radius: width / 2
                color: Qt.alpha(Kirigami.Theme.textColor, backButton.down ? 0.16 : backButton.hovered ? 0.10 : 0.05)
                border.width: backButton.visualFocus ? 2 : 1
                border.color: backButton.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.18)
                Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
            }
            ToolTip.visible: hovered
            ToolTip.text: text
            ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        Label {
            text: root.title
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Kirigami.ActionToolBar {
            visible: root.actions.length > 0
            actions: root.actions
            alignment: Qt.AlignRight
        }
    }
}
