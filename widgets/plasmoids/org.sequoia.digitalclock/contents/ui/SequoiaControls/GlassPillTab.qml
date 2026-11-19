// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Templates as T
import org.kde.kirigami as K

T.TabButton {
    id: control
    readonly property var switcher: T.TabBar.tabBar
    readonly property color textColor: switcher ? switcher.textColor : K.Theme.textColor
    readonly property color accentColor: switcher ? switcher.accentColor : K.Theme.highlightColor
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
    leftPadding: 12; rightPadding: 12
    topPadding: 8; bottomPadding: 8
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    contentItem: C.Label {
        text: control.text
        font: control.font
        color: control.textColor
        opacity: control.enabled ? 1 : .45
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: height / 2
        color: Qt.alpha(control.textColor, control.down ? .10 : control.hovered ? .05 : 0)
        border.width: control.activeFocus || (control.checked && control.switcher && control.switcher.visualFocus) ? 2 : 0
        border.color: control.accentColor
    }
    Accessible.onPressAction: animateClick()
    Keys.onReturnPressed: animateClick()
    Keys.onEnterPressed: animateClick()
}
