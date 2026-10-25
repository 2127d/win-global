// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import "qrc:/sequoia-weather/ActionIcons.js" as ActionIcons

ToolButton {
    id: control
    implicitWidth: 32
    implicitHeight: 32
    padding: 0
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    text: i18n("Close")
    Accessible.name: text
    readonly property bool crossVisible: hovered || down || visualFocus
    background: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 24; height: 24; radius: 12
            color: "transparent"
            border.width: control.visualFocus ? 2 : 0
            border.color: Kirigami.Theme.highlightColor
        }
    }
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 16; height: 16; radius: 8
            color: control.down ? "#e0443e" : "#ff5f57"
            border.width: 1
            border.color: "#d94c45"
            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
        }
        Kirigami.Icon {
            anchors.centerIn: parent
            width: 10; height: 10
            source: ActionIcons.source("window-close-symbolic")
            isMask: true
            color: "#710e09"
            opacity: control.crossVisible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
        }
    }
    ToolTip.visible: hovered
    ToolTip.text: text
    ToolTip.delay: Kirigami.Units.toolTipDelay
}
