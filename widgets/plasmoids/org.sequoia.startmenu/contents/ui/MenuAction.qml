// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PC.AbstractButton {
    id: control
    property string symbol: ""
    property string trailingSymbol: ""
    property bool iconOnly: false
    property bool raised: false
    property bool selected: false
    property int motionDuration: Kirigami.Units.shortDuration > 1 ? Kirigami.Units.shortDuration : 0
    implicitHeight: 36
    implicitWidth: iconOnly ? implicitHeight : actionContent.implicitWidth + 24
    padding: 10
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    font.pixelSize: 12
    Accessible.name: text
    background: MenuSurface {
        radius: control.iconOnly || control.raised ? height / 2 : 12
        color: control.selected ? Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.highlightColor, 0.16))
            : control.raised ? Qt.tint(Kirigami.Theme.backgroundColor,
            Qt.alpha(Kirigami.Theme.textColor, control.down ? 0.14 : control.hovered ? 0.08 : 0))
            : Qt.alpha(Kirigami.Theme.textColor, control.down ? 0.14 : control.hovered ? 0.08 : 0)
        border.width: control.visualFocus ? 2 : control.raised ? 1 : 0
        border.color: control.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.13)
        elevated: control.raised
        motionDuration: control.motionDuration
    }
    contentItem: RowLayout {
        id: actionContent
        spacing: 7
        Kirigami.Icon {
            visible: control.symbol !== ""
            source: control.symbol ? Qt.resolvedUrl("../icons/" + control.symbol + ".svg") : ""
            implicitWidth: 18; implicitHeight: 18
            Layout.alignment: Qt.AlignCenter
            isMask: true
            color: Kirigami.Theme.textColor
        }
        Text {
            visible: !control.iconOnly
            text: control.text
            font: control.font
            color: Kirigami.Theme.textColor
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        Kirigami.Icon {
            visible: control.trailingSymbol !== ""
            source: control.trailingSymbol ? Qt.resolvedUrl("../icons/" + control.trailingSymbol + ".svg") : ""
            implicitWidth: 16; implicitHeight: 16
            isMask: true
            color: Kirigami.Theme.textColor
        }
    }
    PC.ToolTip.visible: hovered && iconOnly
    PC.ToolTip.text: text
    PC.ToolTip.delay: 600
}
