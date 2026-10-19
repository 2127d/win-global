// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.sequoia.login 1.0

FocusScope {
    id: picker
    required property var scene
    property bool opened: false
    property int highlightedIndex: 0
    readonly property real unit: scene.loginUnit
    readonly property var accounts: scene.backend.users.concat([{manual: true, name: "", realName: qsTr("Other User"), icon: ""}])
    signal closed()
    visible: opened
    enabled: !scene.backend.busy
    anchors.centerIn: parent
    width: Math.min(628 * unit, scene.width - 48 * scene.unit)
    height: 278 * unit

    function open() {
        if (scene.backend.busy) return;
        highlightedIndex = scene.backend.manual ? accounts.length - 1 : Math.max(0, Math.min(scene.backend.currentIndex, accounts.length - 1));
        opened = true;
        forceActiveFocus(Qt.OtherFocusReason);
        focusSelection();
        Qt.inputMethod.hide();
        Qt.callLater(focusSelection);
    }
    function close() {
        if (!opened) return;
        opened = false;
        closed();
    }
    function focusSelection() {
        if (!opened) return;
        highlightedIndex = Math.max(0, Math.min(highlightedIndex, accounts.length - 1));
        const button = choices.itemAt(highlightedIndex);
        if (button) {
            button.forceActiveFocus(Qt.OtherFocusReason);
            reveal(button);
        }
    }
    function reveal(button) {
        if (button.x < viewport.contentX) viewport.contentX = button.x;
        else if (button.x + button.width > viewport.contentX + viewport.width)
            viewport.contentX = button.x + button.width - viewport.width;
    }
    function navigate(offset) {
        highlightedIndex = Math.max(0, Math.min(accounts.length - 1, highlightedIndex + offset));
        focusSelection();
    }
    function choose(index) {
        if (!opened || scene.backend.busy || index < 0 || index >= accounts.length) return;
        if (accounts[index].manual) scene.backend.selectManual();
        else scene.backend.selectUser(index);
        close();
    }
    onAccountsChanged: { if (opened) Qt.callLater(focusSelection); }
    Keys.onEscapePressed: close()
    Keys.onLeftPressed: navigate(-1)
    Keys.onRightPressed: navigate(1)
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Home || event.key === Qt.Key_End) {
            highlightedIndex = event.key === Qt.Key_Home ? 0 : accounts.length - 1;
            focusSelection();
            event.accepted = true;
        }
    }

    WallpaperGlass {
        id: glass
        objectName: "userSwitcherGlass"
        anchors.fill: parent
        source: picker.scene.wallpaper
        sceneSize: Qt.size(picker.scene.width, picker.scene.height)
        sampleRect: Qt.rect(picker.x, picker.y, width, height)
        blurRadius: 32 * picker.unit
        radius: 18 * picker.unit
        tint: "#58121d22"
        Rectangle { anchors.fill: parent; radius: glass.radius; color: "transparent"; border.color: "#407b929c" }
    }
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 23 * picker.unit
        spacing: 7 * picker.unit
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 29 * picker.unit; height: width
            source: "../assets/users.svg"
            sourceSize.width: width; sourceSize.height: height
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Switch User"); color: "white"; font.pixelSize: 21 * picker.unit
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Select a user to sign in"); color: "#d7dfe3"; font.pixelSize: 11 * picker.unit
        }
    }
    Controls.AbstractButton {
        id: cancel
        objectName: "cancelUserSwitcher"
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 12 * picker.unit
        width: 25 * picker.unit; height: width; padding: 0
        text: qsTr("Back to sign in"); Accessible.name: text
        focusPolicy: Qt.StrongFocus
        background: Rectangle {
            radius: width / 2; color: cancel.hovered ? "#20ffffff" : "transparent"
            border.color: "#a9d5ed"; border.width: cancel.visualFocus ? 1 : 0
        }
        contentItem: Item {
            Image { anchors.centerIn: parent; width: 12 * picker.unit; height: width; source: "../assets/status-close.svg"; sourceSize.width: width; sourceSize.height: height }
        }
        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: 700
        onClicked: picker.close()
    }
    Flickable {
        id: viewport
        objectName: "userSwitcherViewport"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 124 * picker.unit
        width: Math.min(contentWidth, picker.width - 88 * picker.unit)
        height: 126 * picker.unit
        contentWidth: accountRow.width; contentHeight: height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.HorizontalFlick
        Row {
            id: accountRow
            Repeater {
                id: choices
                model: picker.accounts
                Controls.AbstractButton {
                    id: account
                    required property var modelData
                    required property int index
                    readonly property bool selected: !picker.scene.backend.manual && !modelData.manual && index === picker.scene.backend.currentIndex
                    readonly property bool highlighted: activeFocus || hovered || selected
                    objectName: "userChoice" + index
                    width: 108 * picker.unit; height: viewport.height
                    padding: 0; focusPolicy: Qt.StrongFocus
                    text: modelData.realName || modelData.name
                    Accessible.name: text
                    Accessible.description: selected ? qsTr("Current user") : qsTr("Select user")
                    onActiveFocusChanged: {
                        if (activeFocus) { picker.highlightedIndex = index; picker.reveal(account); }
                    }
                    onClicked: picker.choose(index)
                    Keys.onReturnPressed: picker.choose(index)
                    Keys.onEnterPressed: picker.choose(index)
                    contentItem: Item {
                        Rectangle {
                            id: ring
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: (86 * picker.unit - height) / 2
                            width: (account.highlighted ? 84 : 72) * picker.unit; height: width
                            radius: width / 2
                            color: account.highlighted ? "#202da1f6" : "#25212f38"
                            border.color: account.highlighted ? "#2da1f6" : "#8099a5"
                            border.width: account.highlighted ? 2.5 * picker.unit : 1
                            Avatar {
                                anchors.fill: parent; anchors.margins: 4 * picker.unit
                                visible: !account.modelData.manual
                                name: account.text; source: account.modelData.icon || ""
                            }
                            Image {
                                anchors.centerIn: parent; width: 40 * picker.unit; height: width
                                visible: account.modelData.manual === true
                                source: "../assets/user-outline.svg"; sourceSize.width: width; sourceSize.height: height
                            }
                        }
                        Text {
                            anchors.top: parent.top; anchors.topMargin: 98 * picker.unit
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 8 * picker.unit
                            text: account.text; color: "white"; font.pixelSize: 11 * picker.unit
                            horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
                        }
                    }
                    Controls.ToolTip.text: text
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.delay: 700
                }
            }
        }
    }
    Repeater {
        model: [-1, 1]
        Controls.AbstractButton {
            id: navigation
            required property int modelData
            objectName: modelData < 0 ? "previousUser" : "nextUser"
            x: modelData < 0 ? 12 * picker.unit : picker.width - width - 12 * picker.unit
            y: viewport.y + 43 * picker.unit - height / 2
            width: 28 * picker.unit; height: width; padding: 0
            enabled: modelData < 0 ? picker.highlightedIndex > 0 : picker.highlightedIndex < picker.accounts.length - 1
            opacity: enabled ? 1 : 0.3
            text: modelData < 0 ? qsTr("Previous user") : qsTr("Next user")
            Accessible.name: text; focusPolicy: Qt.StrongFocus
            background: Rectangle {
                radius: width / 2; color: navigation.hovered ? "#35ffffff" : "#18ffffff"
                border.width: navigation.visualFocus ? 1 : 0; border.color: "#a9d5ed"
            }
            contentItem: Item {
                Image {
                    anchors.centerIn: parent; width: 14 * picker.unit; height: width
                    source: "../assets/chevron-right.svg"; rotation: navigation.modelData < 0 ? 180 : 0
                    sourceSize.width: width; sourceSize.height: height
                }
            }
            onClicked: picker.navigate(modelData)
        }
    }
}
