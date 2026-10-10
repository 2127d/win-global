// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import "../qml" as Theme

Item {
    id: testRoot
    width: 1280; height: 800
    QtObject {
        id: mock
        property var users: [
            {name: "alice", realName: "Alice Example", icon: "", needsPassword: true},
            {name: "guest", realName: "Guest", icon: "", needsPassword: false}
        ]
        property int currentIndex: 0
        property bool manual: false
        property string username: ""
        property string password: ""
        property bool showPassword: false
        property bool capsLock: false
        property bool canPowerOff: true
        property bool canReboot: true
        property bool canSuspend: true
        property int sleeps: 0
        property bool busy: false
        property string message: ""
        property string submittedUser: ""
        property string submittedPassword: ""
        property int requests: 0
        property int shutdowns: 0
        property int reboots: 0
        function setPassword(value) { password = value; }
        function setUsername(value) { username = value; }
        function setShowPassword(value) { showPassword = value; }
        function clearPassword() { password = ""; showPassword = false; }
        function selectUser(index) { clearPassword(); currentIndex = index; manual = false; }
        function selectManual() { clearPassword(); manual = true; }
        function login(user, secret) { busy = true; submittedUser = user; submittedPassword = secret; requests++; }
        function fail() { clearPassword(); message = "Login failed. Please try again."; busy = false; }
        function powerOff() { shutdowns++; }
        function suspend() { sleeps++; }
        function reboot() { reboots++; }
    }
    StatusFixture { id: statusFixture }
    Theme.LoginScreen { id: screen; anchors.fill: parent; backend: mock }
    TestCase {
        name: "SequoiaLogin"
        when: windowShown
        function init() {
            screen.systemStatus = null;
            testRoot.width = 1280; testRoot.height = 800;
            mock.busy = false; mock.manual = false; mock.currentIndex = 0;
            mock.username = ""; mock.clearPassword(); mock.message = ""; mock.requests = 0;
            mock.canPowerOff = true; mock.canReboot = true; mock.canSuspend = true; mock.capsLock = false;
            screen.userMenu.close(); screen.focusInput(); wait(30);
        }
        function test_password_and_failure() {
            verify(screen.passwordField.activeFocus);
            keyClick(Qt.Key_A); keyClick(Qt.Key_B);
            compare(mock.password, "ab");
            keyClick(Qt.Key_Return);
            compare(mock.requests, 1);
            compare(mock.submittedUser, "alice"); compare(mock.submittedPassword, "ab");
            verify(!screen.loginPanel.enabled); verify(!screen.powerControls.enabled);
            screen.submit(); compare(mock.requests, 1);
            mock.fail(); wait(10);
            compare(screen.passwordField.text, "");
            verify(screen.passwordField.activeFocus);
            verify(screen.errorLabel.text.indexOf("Login failed") >= 0);
            keyClick(Qt.Key_C); keyClick(Qt.Key_Return); compare(mock.requests, 2);
        }
        function test_status_popup_returns_to_password() {
            screen.systemStatus = statusFixture;
            mock.setPassword("sample");
            screen.statusIsland.showSection("audio");
            tryCompare(screen.statusIsland.popup, "opened", true);
            screen.statusIsland.popup.close();
            tryCompare(screen.statusIsland.popup, "opened", false);
            tryCompare(screen.passwordField, "activeFocus", true);
            compare(mock.password, "sample");
        }
        function test_login_button_and_account_name() {
            const profile = findChild(screen, "profileButton");
            compare(profile.text, "Alice Example");
            mock.setPassword("sample");
            const button = findChild(screen, "signIn");
            verify(button.visible);
            mouseClick(button, button.width / 2, button.height / 2);
            compare(mock.submittedUser, "alice"); compare(mock.submittedPassword, "sample");
            mock.fail(); mock.selectUser(1);
            compare(profile.text, "Guest");
        }
        function test_live_clock_and_date() {
            screen.currentDateTime = new Date(2026, 8, 27, 23, 59, 59);
            const previousDay = screen.dateLabel.text;
            screen.currentDateTime = new Date(2026, 8, 28, 0, 0, 0);
            verify(screen.dateLabel.text !== previousDay);
            compare(screen.clockLabel.text, Qt.formatTime(screen.currentDateTime, "HH:mm"));
            compare(screen.dateLabel.text, Qt.formatDate(screen.currentDateTime, "dddd, MMMM d"));
            // The running timer must replace a synthetic time with the real clock.
            screen.currentDateTime = new Date(2000, 0, 1);
            tryVerify(() => Math.abs(new Date().getTime() - screen.currentDateTime.getTime()) < 2000, 2500);
        }
        function test_avatar_image() {
            const avatar = findChild(screen, "accountAvatar");
            avatar.source = Qt.resolvedUrl("avatar.svg");
            tryCompare(avatar, "hasImage", true);
            waitForRendering(avatar);
            // Capture the scene: QtTest's item crop misses nested/reparented items.
            const picture = grabImage(screen);
            const center = avatar.mapToItem(screen, avatar.width / 2, avatar.height / 2);
            const corner = avatar.mapToItem(screen, 0, 0);
            compare(picture.red(center.x, center.y), 208);
            compare(picture.green(center.x, center.y), 32);
            verify(picture.green(corner.x, corner.y) !== 32);
            avatar.source = "";
        }
        function test_reveal_and_escape() {
            mock.setPassword("sample");
            const eye = findChild(screen, "revealPassword");
            mouseClick(eye);
            verify(mock.showPassword);
            screen.passwordField.forceActiveFocus(); keyClick(Qt.Key_Escape);
            compare(mock.password, ""); verify(!mock.showPassword);
            keyClick(Qt.Key_Tab); verify(eye.activeFocus);
        }
        function test_account_picker_and_passwordless() {
            mouseClick(findChild(screen, "switchUser"));
            tryCompare(screen.userMenu, "opened", true);
            keyClick(Qt.Key_Escape); tryCompare(screen.userMenu, "opened", false); tryCompare(screen.userMenu, "visible", false);
            mock.setPassword("old secret");
            screen.userMenu.open(); waitForRendering(screen);
            mouseClick(findChild(screen.userMenu, "userChoice1"));
            tryCompare(screen.userMenu, "opened", false);
            compare(mock.password, ""); verify(!screen.passwordRequired);
            const button = findChild(screen, "signIn"); verify(button.visible);
            mouseClick(button, button.width / 2, button.height / 2); compare(mock.submittedUser, "guest"); compare(mock.submittedPassword, "");
        }
        function test_switcher_replaces_only_card_and_focuses_password() {
            screen.systemStatus = statusFixture;
            const outer = [screen.clockLabel, screen.dateLabel, screen.powerControls, screen.statusIsland];
            const positions = outer.map(item => item.mapToItem(screen, 0, 0));
            mock.selectUser(1);
            mouseClick(findChild(screen, "switchUser"));
            tryCompare(screen.userMenu, "opened", true);
            verify(!screen.loginCard.visible);
            verify(!screen.passwordField.activeFocus);
            screen.submit(); compare(mock.requests, 0);
            for (let i = 0; i < outer.length; ++i) {
                verify(outer[i].visible);
                compare(outer[i].mapToItem(screen, 0, 0), positions[i]);
            }
            mouseClick(findChild(screen.userMenu, "userChoice0"));
            tryCompare(screen.passwordField, "activeFocus", true);
            verify(screen.loginCard.visible); verify(!screen.userMenu.visible);
            compare(mock.currentIndex, 0);
            compare(findChild(screen, "profileButton").text, "Alice Example");
            keyClick(Qt.Key_X); keyClick(Qt.Key_Return);
            compare(mock.submittedUser, "alice"); compare(mock.submittedPassword, "x");
        }
        function test_switcher_keyboard_manual_and_cancel() {
            mock.setPassword("previous secret"); mock.setShowPassword(true);
            screen.userMenu.open(); wait(30);
            keyClick(Qt.Key_End);
            keyClick(Qt.Key_Return);
            tryCompare(screen.usernameField, "activeFocus", true);
            verify(mock.manual); compare(mock.password, ""); verify(!mock.showPassword);
            keyClick(Qt.Key_B); keyClick(Qt.Key_Return); keyClick(Qt.Key_X);
            compare(mock.username, "b"); compare(mock.password, "x");
            screen.userMenu.open(); wait(30);
            keyClick(Qt.Key_Home); keyClick(Qt.Key_Escape);
            tryCompare(screen.passwordField, "activeFocus", true);
            verify(mock.manual); compare(mock.password, "x");
            screen.userMenu.open(); wait(30);
            mouseClick(findChild(screen.userMenu, "cancelUserSwitcher"));
            tryCompare(screen.userMenu, "opened", false);
        }
        function test_switcher_scrolls_many_accounts() {
            const original = mock.users;
            try {
                mock.users = Array.from({length: 12}, (_, i) => ({name: "user" + i, realName: "Account " + i, icon: "", needsPassword: true}));
                testRoot.width = 800; testRoot.height = 480;
                screen.userMenu.open(); wait(30);
                keyClick(Qt.Key_End);
                const viewport = findChild(screen.userMenu, "userSwitcherViewport");
                verify(viewport.contentX > 0);
                keyClick(Qt.Key_Left); keyClick(Qt.Key_Return);
                tryCompare(screen.passwordField, "activeFocus", true);
                compare(mock.currentIndex, 11);
                screen.userMenu.open(); wait(30);
                keyClick(Qt.Key_Home); compare(viewport.contentX, 0);
            } finally {
                screen.userMenu.close(); mock.currentIndex = 0; mock.users = original;
            }
        }
        function test_manual_login() {
            mock.selectManual(); wait(10);
            verify(screen.usernameField.activeFocus);
            keyClick(Qt.Key_B); keyClick(Qt.Key_O); keyClick(Qt.Key_B); keyClick(Qt.Key_Return);
            verify(screen.passwordField.activeFocus);
            keyClick(Qt.Key_X); keyClick(Qt.Key_Return);
            compare(mock.submittedUser, "bob"); compare(mock.submittedPassword, "x");
        }
        function test_power_and_caps_lock() {
            const off = findChild(screen, "shutdown"); const restart = findChild(screen, "restart");
            const sleep = findChild(screen, "sleep");
            mouseClick(off); mouseClick(restart); mouseClick(sleep);
            compare(mock.shutdowns, 1); compare(mock.reboots, 1);
            compare(mock.sleeps, 1);
            mock.canPowerOff = false; mock.canReboot = false; mock.canSuspend = false;
            mouseClick(off); mouseClick(restart); mouseClick(sleep);
            compare(mock.sleeps, 1);
            compare(mock.shutdowns, 1); compare(mock.reboots, 1);
            mock.capsLock = true; compare(screen.errorLabel.text, "Caps Lock is on");
        }
        function test_responsive_data() {
            return [{tag: "small", w: 800, h: 480}, {tag: "template", w: 1024, h: 536}, {tag: "reference", w: 1216, h: 640},
                    {tag: "fullhd", w: 1920, h: 1080}, {tag: "4k", w: 3840, h: 2160}];
        }
        function test_responsive(data) {
            testRoot.width = data.w; testRoot.height = data.h; wait(20);
            const field = screen.passwordField.mapToItem(screen, 0, 0);
            const power = screen.powerControls;
            verify(field.y + screen.passwordField.height <= screen.height - 10);
            verify(field.x + screen.passwordField.width < power.x);
            verify(screen.loginCard.y >= 0 && screen.loginCard.y + screen.loginCard.height <= screen.height);
            verify(power.x + power.width < screen.width);
            for (const name of ["sleep", "restart", "shutdown"]) {
                const glass = findChild(screen, name + "Glass");
                const position = glass.mapToItem(screen, 0, 0);
                fuzzyCompare(glass.sampleRect.x, position.x, 0.1);
                fuzzyCompare(glass.sampleRect.y, position.y, 0.1);
                compare(glass.sampleRect.width, glass.width);
            }
            mock.selectManual(); wait(20);
            verify(screen.loginCard.y >= 0 && screen.loginCard.y + screen.loginCard.height <= screen.height);
            screen.userMenu.open(); wait(20);
            const picker = screen.userMenu;
            verify(picker.x >= 0 && picker.x + picker.width <= screen.width);
            verify(picker.y > screen.statusIsland.y + screen.statusIsland.height);
            verify(picker.y + picker.height < screen.clockLabel.mapToItem(screen, 0, 0).y);
            const glass = findChild(picker, "userSwitcherGlass");
            compare(glass.sampleRect.x, picker.x); compare(glass.sampleRect.y, picker.y);
        }
    }
}
