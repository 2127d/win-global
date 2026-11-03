// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import "SequoiaPopup" as Popup
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
    background: Popup.ActionSurface {
        radius: control.iconOnly || control.raised ? height / 2 : Popup.Tokens.controlRadius
        emphasized: control.selected
        pressed: control.down
        hovered: control.hovered && control.enabled
        focused: control.visualFocus
        raised: control.raised
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
