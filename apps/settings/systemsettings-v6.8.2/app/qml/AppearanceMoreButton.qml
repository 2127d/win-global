// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
import org.sequoia.components 1.0 as S

AppearanceActionPill {
    id: control
    objectName: "appearanceMoreButton"
    required property Item backdropItem
    property bool componentsVisible: true
    property alias menu: menu
    signal toggleComponents()
    Layout.minimumWidth: 42
    Layout.maximumWidth: 42
    Layout.minimumHeight: 42
    Layout.maximumHeight: 42
    implicitWidth: 42
    implicitHeight: 42
    leftPadding: 12
    rightPadding: 12
    Accessible.name: qsTr("More appearance options")
    contentItem: C.Label {
        text: "⋯"
        font.pixelSize: 24
        color: K.Theme.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    onClicked: menu.visible ? menu.close() : menu.open()
    C.ToolTip.visible: hovered && !menu.visible
    C.ToolTip.text: qsTr("More appearance options")

    component RoundedMenuItem: AppearanceMenuItem {
        id: menuItem
        background: S.PillHighlight {
            theme: menuItem.theme
            highlighted: menuItem.highlighted
            hovered: menuItem.hovered
            pressed: menuItem.down
            focused: menuItem.visualFocus
            enabled: menuItem.enabled
        }
    }

    AppearancePopupMenu {
        id: menu
        objectName: "appearanceMoreMenu"
        trigger: control
        backdropItem: control.backdropItem
        RoundedMenuItem {
            text: control.componentsVisible ? qsTr("Hide appearance components") : qsTr("Show appearance components")
            onTriggered: control.toggleComponents()
        }
        RoundedMenuItem {
            text: qsTr("Undo last applied look")
            enabled: !appearancePreview.busy
            onTriggered: appearancePreview.trial("restore")
        }
        RoundedMenuItem {
            text: qsTr("Global Theme…")
            onTriggered: navigation.open("kcm_lookandfeel")
        }
    }
}
