// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts

FocusScope {
    id: session
    readonly property real unit: lockScreenUi.loginUnit
    readonly property var themeColors: lockScreenUi.themeColors
    property alias notificationMessage: notification.text
    property alias actionItems: actions.children
    property alias actionItemsVisible: actions.visible
    property alias userListModel: users.model
    property alias userListCurrentIndex: users.currentIndex
    property alias userListCurrentItem: users.currentItem
    property alias userList: users
    property alias mediaArea: media
    property bool showUserList: true
    default property alias _children: prompts.data
    signal userSelected()
    function playHighlightAnimation() { highlight.restart(); }

    SequoiaGlass {
        id: card
        objectName: "unlockCard"
        anchors.centerIn: parent
        width: 252 * session.unit - 4
        height: users.height + prompts.implicitHeight + 66 * session.unit - 4
        radius: 18 * session.unit
        sceneSource: wallpaper
        scene: lockScreenUi
        themeColors: session.themeColors
        blurRadius: 32 * session.unit

        ListView {
            id: users
            // Preserve the SessionManagementScreen interface used by MainBlock.
            x: 32 * session.unit
            y: 30 * session.unit - 2
            width: card.width - 64 * session.unit
            height: 100 * session.unit
            interactive: false
            visible: session.showUserList
            delegate: Item {
                id: profile
                required property string name
                required property string realName
                required property string icon
                width: users.width; height: users.height
                SequoiaAvatar {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 68 * session.unit; height: width
                    name: profile.realName || profile.name
                    source: profile.icon
                    Rectangle { anchors.fill: parent; radius: width / 2; color: "transparent"; border.color: session.themeColors.border; border.width: 2 }
                }
                Text {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    text: profile.realName || profile.name
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    color: session.themeColors.foreground
                    font.pixelSize: 15 * session.unit
                }
            }
        }
        ColumnLayout {
            id: prompts
            anchors.top: users.bottom
            anchors.topMargin: 14 * session.unit
            anchors.horizontalCenter: parent.horizontalCenter
            width: card.width - 64 * session.unit
            spacing: 8 * session.unit
        }
    }
    Text {
        id: notification
        objectName: "unlockMessage"
        anchors.top: card.bottom
        anchors.topMargin: 12 * lockScreenUi.unit
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 32, 360 * lockScreenUi.unit)
        visible: text.length > 0
        color: "white"
        font.pixelSize: 15 * lockScreenUi.unit
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        style: Text.Outline; styleColor: "#80000000"
        Accessible.role: Accessible.AlertMessage
        Accessible.name: text
        SequentialAnimation {
            id: highlight
            NumberAnimation { target: notification; property: "scale"; to: 1.1; duration: 120 }
            NumberAnimation { target: notification; property: "scale"; to: 1; duration: 120 }
        }
    }
    Item {
        id: media
        anchors.horizontalCenter: parent.horizontalCenter
        y: card.y + card.height + (notification.visible ? notification.height + 24 : 20) * lockScreenUi.unit
        width: Math.min(392, parent.width - 32)
        height: 156
        visible: y + height < parent.height - 12
    }
    SequoiaGlass {
        x: actions.x - 10 * lockScreenUi.powerUnit
        y: actions.y - 10 * lockScreenUi.powerUnit
        width: actions.width + 20 * lockScreenUi.powerUnit
        height: actions.height + 16 * lockScreenUi.powerUnit
        radius: 18 * lockScreenUi.powerUnit
        sceneSource: wallpaper
        scene: lockScreenUi
        themeColors: session.themeColors
        visible: actions.visible && actions.visibleChildren.length > 0
    }
    Row {
        id: actions
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24 * lockScreenUi.unit
    }
}
