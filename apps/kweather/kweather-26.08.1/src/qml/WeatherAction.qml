// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "qrc:/sequoia-weather/ActionIcons.js" as ActionIcons

Button {
    id: control
    property bool atmospheric: false
    property bool selected: false
    property string symbol: ""
    property bool iconOnly: false
    property bool pill: false
    property bool floating: false
    property color floatingSurface: atmospheric ? "#344f65" : Kirigami.Theme.backgroundColor
    property color ink: atmospheric ? "#f5f8ff" : Kirigami.Theme.textColor
    implicitHeight: 38
    implicitWidth: iconOnly ? implicitHeight : contentItem.implicitWidth + 24
    padding: 10
    hoverEnabled: true
    Accessible.name: text
    background: Rectangle {
        id: buttonSurface
        radius: control.pill || control.floating ? height / 2 : 12
        RectangularShadow {
            objectName: "weatherActionShadow"
            anchors.fill: parent
            z: -1
            visible: control.floating && GraphicsInfo.api !== GraphicsInfo.Software
            radius: buttonSurface.radius
            blur: 12
            offset: Qt.vector2d(0, 4)
            color: Qt.rgba(0, 0, 0, 0.24)
        }
        color: control.floating ? Qt.tint(control.floatingSurface, Qt.alpha(control.ink, control.down ? 0.14 : control.hovered ? 0.08 : 0))
              : control.selected ? (control.atmospheric ? "#345776" : Qt.alpha(Kirigami.Theme.highlightColor, 0.16))
              : control.down ? Qt.alpha(control.ink, 0.14) : control.hovered ? Qt.alpha(control.ink, 0.08) : "transparent"
        border.width: control.visualFocus ? 2 : control.floating ? 1 : 0
        border.color: control.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(control.ink, 0.13)
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }
    contentItem: RowLayout {
        spacing: 7
        Kirigami.Icon { Layout.alignment: control.iconOnly ? Qt.AlignCenter : Qt.AlignVCenter; visible: control.symbol !== ""; source: ActionIcons.source(control.symbol); implicitWidth: 18; implicitHeight: 18; color: control.ink; isMask: true }
        Label { visible: !control.iconOnly; text: control.text; color: control.ink; font.weight: Font.Medium }
    }
    ToolTip.visible: hovered && iconOnly
    ToolTip.text: text
    ToolTip.delay: 600
}
