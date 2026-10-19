// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.login as PlasmaLogin
import org.kde.plasma.private.keyboardindicator as KeyboardIndicator

LoginScreen {
    id: root
    anchors.fill: parent
    backend: native
    systemStatus: SystemStatus {}
    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false
    wallpaper: "../assets/sequoia-forest.jpg"
    Item {
        id: native
        property var users: []
        readonly property int currentIndex: PlasmaLogin.GreeterState.userListIndex
        readonly property bool manual: PlasmaLogin.GreeterState.loginState === PlasmaLogin.GreeterState.LoginState.UserPrompt || users.length === 0
        readonly property string username: PlasmaLogin.GreeterState.userPromptUsername
        readonly property string password: manual ? PlasmaLogin.GreeterState.userPromptPassword : PlasmaLogin.GreeterState.userListPassword
        readonly property bool showPassword: PlasmaLogin.GreeterState.showPassword
        readonly property bool capsLock: caps.locked
        readonly property bool canPowerOff: PlasmaLogin.SessionManagement.canShutdown
        readonly property bool canSuspend: PlasmaLogin.SessionManagement.canSuspend
        readonly property bool canReboot: PlasmaLogin.SessionManagement.canReboot
        readonly property bool busy: ViewState.busy
        readonly property string message: ViewState.message
        function refreshUsers() {
            let values = [];
            for (let i = 0; i < accounts.count; ++i) {
                const account = accounts.objectAt(i);
                if (account) values.push({name: account.username, realName: account.realName, icon: account.icon, needsPassword: account.needsPassword});
            }
            users = values;
        }
        function setPassword(value) {
            if (manual) PlasmaLogin.GreeterState.userPromptPassword = value;
            else PlasmaLogin.GreeterState.userListPassword = value;
            ViewState.message = "";
        }
        function setUsername(value) { PlasmaLogin.GreeterState.userPromptUsername = value; }
        function setShowPassword(value) { PlasmaLogin.GreeterState.showPassword = value; }
        function clearPassword() { PlasmaLogin.GreeterState.clearPasswords(); setShowPassword(false); }
        function selectUser(index) {
            clearPassword();
            PlasmaLogin.GreeterState.userListIndex = index;
            PlasmaLogin.GreeterState.loginState = PlasmaLogin.GreeterState.LoginState.UserList;
            ViewState.message = "";
        }
        function selectManual() {
            clearPassword();
            PlasmaLogin.GreeterState.loginState = PlasmaLogin.GreeterState.LoginState.UserPrompt;
            ViewState.message = "";
        }
        function login(username, password) {
            if (busy) return;
            ViewState.busy = true; ViewState.message = "";
            PlasmaLogin.GreeterState.handleLoginRequest(username, password, session.currentSessionType, session.currentSessionFileName);
        }
        function suspend() { PlasmaLogin.SessionManagement.suspend(); }
        function powerOff() { PlasmaLogin.SessionManagement.requestShutdown(PlasmaLogin.SessionManagement.ConfirmationMode.Skip); }
        function reboot() { PlasmaLogin.SessionManagement.requestReboot(PlasmaLogin.SessionManagement.ConfirmationMode.Skip); }
        Instantiator {
            id: accounts
            model: PlasmaLogin.UserModel
            delegate: QtObject {
                required property var model
                readonly property string username: model.name
                readonly property string realName: model.realName || ""
                readonly property string icon: model.icon || ""
                readonly property bool needsPassword: model.needsPassword === undefined || model.needsPassword
            }
            onObjectAdded: Qt.callLater(native.refreshUsers)
            onObjectRemoved: Qt.callLater(native.refreshUsers)
        }
        KeyboardIndicator.KeyState { id: caps; key: Qt.Key_CapsLock }
        Connections {
            target: PlasmaLogin.Authenticator
            function onLoginFailed() {
                native.clearPassword();
                ViewState.busy = false;
                ViewState.message = qsTr("Login failed. Please try again.");
                root.focusInput();
            }
            function onLoginSucceeded() { native.clearPassword(); }
        }
    }
    footer: [
        PlasmaLogin.KeyboardButton {
            id: keyboard
            Layout.maximumWidth: root.width * 0.18
            contentItem: Text { text: keyboard.text; color: "white"; font.pixelSize: 12 * root.unit; elide: Text.ElideRight }
        }
    ]
    // Keep the native session model, saved selection and login payload adapter.
    // The visible picker uses the same glass popup as the other tray controls.
    sessionModel: PlasmaLogin.SessionModel
    sessionIndex: session.currentIndex
    sessionDescription: session.text
    onSessionSelected: index => { session.currentIndex = index; root.focusInput(); }
    PlasmaLogin.SessionButton {
        id: session
        visible: false
    }
    Connections {
        target: greeterEventFilter
        function onKeyPressed() { Qt.callLater(() => PlasmaLogin.GreeterState.activateWindow(root.Window.window)); }
        function onEscapeKeyPressed() { native.clearPassword(); root.userMenu.close(); }
    }
    TapHandler { onPressedChanged: { if (pressed) PlasmaLogin.GreeterState.activateWindow(root.Window.window); } }
}
