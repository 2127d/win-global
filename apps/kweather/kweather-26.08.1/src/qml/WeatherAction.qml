// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Button {
    id: control
    property bool atmospheric: false
    property bool selected: false
    property string symbol: ""
    property bool iconOnly: false
    property color ink: atmospheric ? "#f5f8ff" : Kirigami.Theme.textColor
    implicitHeight: 38
    implicitWidth: iconOnly ? 38 : contentItem.implicitWidth + 24
    padding: 10
    hoverEnabled: true
    Accessible.name: text
    background: Rectangle {
        radius: 12
        color: control.selected ? (control.atmospheric ? "#345776" : Qt.alpha(Kirigami.Theme.highlightColor, 0.16))
              : control.down ? Qt.alpha(control.ink, 0.14) : control.hovered ? Qt.alpha(control.ink, 0.08) : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Kirigami.Theme.highlightColor
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }
    contentItem: RowLayout {
        spacing: 7
        Kirigami.Icon { visible: control.symbol !== ""; source: control.symbol; implicitWidth: 18; implicitHeight: 18; color: control.ink; isMask: true }
        Label { visible: !control.iconOnly; text: control.text; color: control.ink; font.weight: Font.Medium }
    }
    ToolTip.visible: hovered && iconOnly
    ToolTip.text: text
    ToolTip.delay: 600
}
