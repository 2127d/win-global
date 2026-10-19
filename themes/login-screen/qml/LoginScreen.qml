// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.sequoia.login 1.0

Item {
    id: screen
    required property var backend
    property url wallpaper: ""
    property var systemStatus: null
    // Sequoia Eleven's blue accent on the light input surfaces.
    readonly property color accentColor: "#2463d4"
    property date currentDateTime: new Date()
    readonly property string clockText: Qt.formatTime(currentDateTime, "HH:mm")
    readonly property string dateText: Qt.formatDate(currentDateTime, "dddd, MMMM d")
    readonly property real unit: Math.max(0.65, Math.min(2, width / 1024, height / 560))
    readonly property real loginUnit: Math.max(0.75, unit * 0.8)
    readonly property real powerUnit: Math.max(0.85, unit * 0.8)
    readonly property var user: backend.users[backend.currentIndex] || {name: "", realName: "", icon: "", needsPassword: true}
    readonly property bool passwordRequired: backend.manual || user.needsPassword
    property alias footer: footerExtras.data
    property alias sessionModel: status.sessionModel
    property alias sessionIndex: status.sessionIndex
    property alias sessionDescription: status.sessionDescription
    signal sessionSelected(int index)
    property alias passwordField: password
    property alias userMenu: userPicker
    property alias loginPanel: panel
    property alias powerControls: power
    property alias usernameField: username
    property alias errorLabel: error
    property alias clockLabel: clockLabel
    property alias dateLabel: dateLabel
    property alias statusIsland: status
    property alias loginCard: card

    function focusInput() {
        if (userPicker.opened) userPicker.focusSelection();
        else if (backend.manual && !username.text.length) username.forceActiveFocus();
        else if (passwordRequired) password.forceActiveFocus();
        else signIn.forceActiveFocus();
    }
    function submit() {
        if (!userPicker.opened && !backend.busy && (backend.manual ? username.text.trim().length : user.name.length))
            backend.login(backend.manual ? username.text.trim() : user.name, password.text);
    }
    Component.onCompleted: Qt.callLater(focusInput)
    onPasswordRequiredChanged: Qt.callLater(focusInput)
    Connections {
        target: screen.backend
        function onBusyChanged() { if (!screen.backend.busy) screen.focusInput(); }
        function onCurrentIndexChanged() { screen.focusInput(); }
        function onManualChanged() { screen.focusInput(); }
    }
    Timer { interval: 1000; repeat: true; running: true; onTriggered: screen.currentDateTime = new Date() }

    Image { anchors.fill: parent; source: screen.wallpaper; fillMode: Image.PreserveAspectCrop; autoTransform: true; asynchronous: true }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#00000000" }
            GradientStop { position: 0.65; color: "#00000000" }
            GradientStop { position: 1; color: "#50080d13" }
        }
    }
    Column {
        id: calendar
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 28 * screen.unit
        anchors.bottomMargin: 22 * screen.unit
        spacing: 6 * screen.unit
        Text {
            id: clockLabel
            objectName: "clock"
            text: screen.clockText
            color: "white"
            font.family: "Adwaita Sans"
            font.pixelSize: 54 * screen.unit
            font.weight: Font.Light
            style: Text.Raised; styleColor: "#40000000"
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
        Text {
            id: dateLabel
            objectName: "date"
            text: screen.dateText
            color: "white"
            font.family: "Adwaita Sans"
            font.pixelSize: 18 * screen.unit
            style: Text.Raised; styleColor: "#40000000"
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
    }
    WallpaperGlass {
        id: card
        objectName: "loginCard"
        visible: !userPicker.opened
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.max(12 * screen.unit, Math.min((screen.height - height) / 2,
            screen.height - (Qt.inputMethod.visible ? Qt.inputMethod.keyboardRectangle.height : 0) - height - 16 * screen.unit))
        width: 252 * screen.loginUnit - 4
        height: panel.implicitHeight + 52 * screen.loginUnit - 4
        radius: 18 * screen.loginUnit
        source: screen.wallpaper
        sceneSize: Qt.size(screen.width, screen.height)
        sampleRect: Qt.rect(x, y, width, height)
        blurRadius: 32 * screen.loginUnit
        tint: "#38121d22"
        Rectangle { anchors.fill: parent; color: "transparent"; radius: card.radius; border.color: "#407b929c"; border.width: 1 }
    }
    Column {
        id: panel
        objectName: "loginPanel"
        parent: card
        anchors.horizontalCenter: parent.horizontalCenter
        y: 30 * screen.loginUnit - 2
        width: parent.width - 64 * screen.loginUnit
        spacing: 14 * screen.loginUnit
        enabled: !screen.backend.busy

        Controls.AbstractButton {
            id: profile
            objectName: "profileButton"
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width; height: 100 * screen.loginUnit
            text: screen.backend.manual ? qsTr("Sign in") : (screen.user.realName || screen.user.name)
            Accessible.name: text + ", " + qsTr("Choose user")
            focusPolicy: Qt.StrongFocus
            onClicked: userPicker.open()
            contentItem: Item {
                Avatar {
                    objectName: "accountAvatar"
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 68 * screen.loginUnit; height: width
                    name: profile.text
                    source: screen.backend.manual ? "" : screen.user.icon
                    Rectangle { anchors.fill: parent; color: "transparent"; radius: width / 2; border.color: profile.visualFocus ? screen.accentColor : "#a7c2c5"; border.width: 2 }
                }
                Text {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    text: profile.text
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    color: "white"; font.pixelSize: 15 * screen.loginUnit
                }
            }
            Controls.ToolTip.text: qsTr("Choose user")
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: 700
        }
        Controls.TextField {
            id: username
            objectName: "username"
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width; height: 28 * screen.loginUnit
            visible: screen.backend.manual
            text: screen.backend.username
            onTextChanged: { if (text !== screen.backend.username) screen.backend.setUsername(text); }
            placeholderText: qsTr("Username")
            Accessible.name: qsTr("Username")
            color: "white"; placeholderTextColor: "#bfc7cb"; font.pixelSize: Math.max(10, 12 * screen.loginUnit)
            topPadding: 3 * screen.loginUnit; bottomPadding: 3 * screen.loginUnit
            leftPadding: 14 * screen.loginUnit; rightPadding: 14 * screen.loginUnit
            background: Rectangle { color: "#30202b30"; radius: 7 * screen.loginUnit; border.color: username.activeFocus ? "#a9d5ed" : "#8099a5"; border.width: username.activeFocus ? 1.5 : 1 }
            onAccepted: screen.focusPassword()
        }
        Item {
            visible: screen.passwordRequired
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width; height: 28 * screen.loginUnit
            Controls.TextField {
                id: password
                objectName: "password"
                anchors.fill: parent
                text: screen.backend.password
                onTextChanged: { if (text !== screen.backend.password) screen.backend.setPassword(text); }
                echoMode: screen.backend.showPassword ? TextInput.Normal : TextInput.Password
                inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                color: "white"; placeholderText: qsTr("Password"); placeholderTextColor: "#bfc7cb"; selectionColor: screen.accentColor; selectedTextColor: "white"
                font.pixelSize: Math.max(10, 12 * screen.loginUnit)
                leftPadding: 28 * screen.loginUnit; rightPadding: 34 * screen.loginUnit
                topPadding: 3 * screen.loginUnit; bottomPadding: 3 * screen.loginUnit
                Accessible.name: qsTr("Password")
                background: Rectangle { color: "#30202b30"; radius: 7 * screen.loginUnit; border.color: password.activeFocus ? "#a9d5ed" : "#8099a5"; border.width: password.activeFocus ? 1.5 : 1 }
                onAccepted: screen.submit()
                Keys.onEscapePressed: { screen.backend.clearPassword(); reveal.checked = false; }
            }
            Image {
                x: 9 * screen.loginUnit; anchors.verticalCenter: parent.verticalCenter
                width: 12 * screen.loginUnit; height: 15 * screen.loginUnit
                source: "../assets/lock.svg"; sourceSize.width: width; sourceSize.height: height
            }
            Controls.AbstractButton {
                id: reveal
                objectName: "revealPassword"
                anchors.right: parent.right; height: parent.height; width: 34 * screen.loginUnit
                checkable: true; checked: screen.backend.showPassword
                onToggled: screen.backend.setShowPassword(checked)
                text: checked ? qsTr("Hide password") : qsTr("Show password")
                Accessible.name: text
                focusPolicy: Qt.StrongFocus
                contentItem: Image { source: screen.backend.showPassword ? "../assets/eye-hidden.svg" : "../assets/eye.svg"; fillMode: Image.Pad; sourceSize.width: 15 * screen.loginUnit; sourceSize.height: 15 * screen.loginUnit }
                background: Rectangle { anchors.margins: 3; color: reveal.checked ? "#505985a3" : "transparent"; radius: height / 2; border.width: reveal.visualFocus ? 1.5 : 0; border.color: screen.accentColor }
                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered
            }
        }
        Row {
            id: loginActions
            objectName: "loginActions"
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12 * screen.loginUnit

            Controls.Button {
                id: signIn
                objectName: "signIn"
                width: signInContents.implicitWidth + 18 * screen.loginUnit
                height: 28 * screen.loginUnit
                text: qsTr("Sign In")
                Accessible.name: text
                padding: 0
                background: Rectangle {
                    color: signIn.down ? "#1685da" : signIn.hovered ? "#48b4ff" : "#2da1f6"
                    radius: 7 * screen.loginUnit
                    border.color: signIn.visualFocus ? "#a9d5ed" : "#8099a5"
                    border.width: 1.5
                }
                contentItem: Item {
                    Row {
                        id: signInContents
                        anchors.centerIn: parent
                        spacing: 6 * screen.loginUnit
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: signIn.text; color: "white"
                            font.pixelSize: Math.max(10, 12 * screen.loginUnit)
                        }
                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 12 * screen.loginUnit; height: width
                            source: "../assets/login.svg"
                            sourceSize.width: width; sourceSize.height: height
                        }
                    }
                }
                onClicked: screen.submit()
            }
            Controls.AbstractButton {
                id: switchUser
                objectName: "switchUser"
                width: switchUserContents.implicitWidth + 10 * screen.loginUnit
                height: signIn.height
                text: qsTr("Switch User")
                Accessible.name: text
                focusPolicy: Qt.StrongFocus
                background: Rectangle { color: switchUser.hovered ? "#15ffffff" : "transparent"; radius: 6 * screen.loginUnit; border.color: "#a9d5ed"; border.width: switchUser.visualFocus ? 1 : 0 }
                contentItem: Item {
                    Row {
                        id: switchUserContents
                        anchors.centerIn: parent
                        spacing: 6 * screen.loginUnit
                        Image { anchors.verticalCenter: parent.verticalCenter; source: "../assets/users.svg"; width: 14 * screen.loginUnit; height: width; sourceSize.width: width; sourceSize.height: height }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: switchUser.text; color: "#eef3f5"; font.pixelSize: Math.max(10, 12 * screen.loginUnit) }
                    }
                }
                onClicked: userPicker.open()
            }
        }
    }
    function focusPassword() { if (screen.passwordRequired) password.forceActiveFocus(); else screen.submit(); }
    Text {
        id: error
        objectName: "loginMessage"
        anchors.horizontalCenter: card.horizontalCenter
        anchors.top: card.bottom; anchors.topMargin: 12 * screen.unit
        width: Math.min(screen.width - 32, 320 * screen.unit)
        text: screen.backend.busy ? qsTr("Signing in…") : (screen.backend.message || (screen.backend.capsLock ? qsTr("Caps Lock is on") : ""))
        visible: text.length > 0 && !userPicker.opened
        color: "white"; font.pixelSize: 15 * screen.unit
        horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
        style: Text.Outline; styleColor: "#55000000"
        Accessible.role: Accessible.AlertMessage; Accessible.name: text
    }
    UserSwitcher {
        id: userPicker
        objectName: "userPicker"
        scene: screen
        onClosed: Qt.callLater(screen.focusInput)
    }
    StatusIsland {
        id: status
        objectName: "statusIsland"
        scene: screen
        status: screen.systemStatus
        anchors.right: parent.right; anchors.top: parent.top
        anchors.rightMargin: 18 * screen.unit
        anchors.topMargin: 8 * screen.unit
        width: implicitWidth; height: implicitHeight
        enabled: !screen.backend.busy
        onDismissed: screen.focusInput()
        onSessionSelected: index => screen.sessionSelected(index)
    }
    RowLayout {
        id: footerExtras
        anchors.left: parent.left; anchors.top: parent.top
        anchors.margins: 18 * screen.unit
        width: Math.max(0, (screen.width - card.width) / 2 - 36 * screen.unit)
        enabled: !screen.backend.busy
    }
    WallpaperGlass {
        objectName: "powerBackdrop"
        x: power.x - 10 * screen.powerUnit; y: power.y - 10 * screen.powerUnit
        width: power.width + 20 * screen.powerUnit; height: power.height + 16 * screen.powerUnit
        source: screen.wallpaper; sceneSize: Qt.size(screen.width, screen.height)
        sampleRect: Qt.rect(x, y, width, height)
        blurRadius: 24 * screen.unit; radius: 18 * screen.powerUnit; tint: "#650b171e"
    }
    Row {
        id: power
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.margins: 24 * screen.unit
        spacing: 0
        enabled: !screen.backend.busy
        PowerButton {
            objectName: "sleep"
            scene: screen
            text: qsTr("Sleep"); symbol: "sleep"
            enabled: screen.backend.canSuspend === true
            onClicked: screen.backend.suspend()
        }
        PowerButton {
            objectName: "restart"
            scene: screen
            text: qsTr("Restart"); symbol: "restart"
            enabled: screen.backend.canReboot
            onClicked: screen.backend.reboot()
        }
        PowerButton {
            objectName: "shutdown"
            scene: screen
            text: qsTr("Shut Down"); symbol: "shutdown"
            enabled: screen.backend.canPowerOff
            onClicked: screen.backend.powerOff()
        }
    }
}
