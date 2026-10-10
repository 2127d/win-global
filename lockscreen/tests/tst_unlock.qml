// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import org.kde.breeze.components

Item {
    id: root
    width: 1280; height: 720
    function i18nd(domain, text) { return text; }
    function i18ndc(domain, context, text) { return text; }
    signal clearPassword()
    signal notificationRepeated()
    property string receivedPassword: ""
    QtObject {
        id: authenticator
        property bool graceLocked: false
        property int authenticatorTypes: 0
        signal noninteractiveError(int kind, var authenticator)
    }
    QtObject {
        id: sessionManagement
        property bool canSwitchUser: true
        property int switches: 0
        function switchUser() { switches++; }
    }
    Rectangle { id: wallpaper; anchors.fill: parent; color: "#567568" }
    Item {
        id: lockScreenUi
        anchors.fill: parent
        readonly property real unit: Math.max(0.65, Math.min(2, width / 1024, height / 560))
        readonly property real loginUnit: Math.max(0.75, unit * 0.8)
        readonly property real powerUnit: Math.max(0.85, unit * 0.8)
        property alias themeColors: appearance
        SequoiaPalette { id: appearance }
        MainBlock {
            id: mainBlock
            anchors.fill: parent
            lockScreenUiVisible: true
            userListModel: ListModel { ListElement { name: "test"; realName: "Test User"; icon: "" } }
            onPasswordResult: password => root.receivedPassword = password
        }
        Loader { id: passwordless; anchors.fill: parent; active: false; source: "NoPasswordUnlock.qml" }
    }
    TestCase {
        name: "UnlockAppearance"
        when: windowShown
        function init() {
            passwordless.active = false;
            mainBlock.visible = true;
            root.width = 1280; root.height = 720;
            authenticator.graceLocked = false;
            mainBlock.lockScreenUiVisible = true;
            mainBlock.showPassword = false;
            root.clearPassword();
            root.receivedPassword = "";
            mainBlock.notificationMessage = "";
            appearance.mode = "dark";
            wait(30);
        }
        function test_passwordAndSubmission() {
            const field = mainBlock.mainPasswordBox;
            field.forceActiveFocus();
            for (const character of "sample password") keyClick(character);
            compare(field.text, "sample password");
            compare(field.echoMode, TextInput.Password);
            mainBlock.showPassword = true;
            compare(field.echoMode, TextInput.Normal);
            mainBlock.showPassword = false;
            keyClick(Qt.Key_Return);
            compare(root.receivedPassword, "sample password");
            root.clearPassword();
            compare(field.text, "");
            verify(field.activeFocus);
        }
        function test_hiddenScreenDoesNotSubmit() {
            mainBlock.lockScreenUiVisible = false;
            mainBlock.mainPasswordBox.text = "sample";
            mainBlock.mainPasswordBox.forceActiveFocus();
            keyClick(Qt.Key_Return);
            compare(root.receivedPassword, "");
        }
        function test_switchUser() {
            const button = findChild(mainBlock, "switchUserButton");
            mouseClick(button);
            compare(sessionManagement.switches, 1);
        }
        function test_paletteAndLayout_data() {
            return [
                {tag: "light-desktop", mode: "light", w: 1280, h: 720},
                {tag: "dark-desktop", mode: "dark", w: 1920, h: 1080},
                {tag: "light-small", mode: "light", w: 800, h: 600},
                {tag: "dark-small", mode: "dark", w: 640, h: 480}
            ];
        }
        function test_paletteAndLayout(data) {
            root.width = data.w; root.height = data.h;
            appearance.mode = data.mode;
            wait(30);
            const card = findChild(mainBlock, "unlockCard");
            const field = mainBlock.mainPasswordBox;
            const button = findChild(mainBlock, "unlockButton");
            const fieldPoint = field.mapToItem(card, 0, 0);
            const buttonPoint = button.mapToItem(card, 0, 0);
            compare(appearance.dark, data.mode === "dark");
            compare(field.color, appearance.foreground);
            verify(card.x >= 0 && card.y >= 0);
            verify(card.x + card.width <= root.width);
            verify(card.y + card.height <= root.height);
            verify(fieldPoint.x >= 0 && fieldPoint.x + field.width <= card.width);
            verify(buttonPoint.y >= fieldPoint.y + field.height);
            verify(buttonPoint.y + button.height <= card.height);
        }
        function test_failureAndGraceLock() {
            mainBlock.notificationMessage = "Unlocking failed";
            const message = findChild(mainBlock, "unlockMessage");
            verify(message.visible);
            compare(message.text, "Unlocking failed");
            authenticator.graceLocked = true;
            verify(!mainBlock.mainPasswordBox.enabled);
        }
        function test_passwordlessLayout() {
            mainBlock.visible = false;
            passwordless.active = true;
            tryCompare(passwordless, "status", Loader.Ready);
            passwordless.item.userListModel = mainBlock.userListModel;
            wait(30);
            const button = findChild(passwordless.item, "noPasswordUnlock");
            verify(button !== null && button.visible);
            verify(button.activeFocus);
        }
    }
}
