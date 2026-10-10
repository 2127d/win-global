#!/usr/bin/env python3
"""Install the lock-screen appearance override from the current system shell."""
import argparse
import os
from pathlib import Path
import shutil
from shell_overrides import MARKERS, COMPACT, patch_launcher

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('The installed Plasma lock screen has changed; review the override before installing.')
    return text.replace(old, new, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path,
                        default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    args = parser.parse_args()
    source = Path('/usr/share/plasma/shells/org.kde.plasma.desktop')
    destination = args.prefix / 'plasma/shells/org.kde.plasma.desktop'
    marker = destination / '.sequoia-eleven-lockscreen'
    launcher_marker = destination / MARKERS['plasma-style']
    keep_launcher = launcher_marker.is_file()
    if destination.exists() and not marker.exists() and not keep_launcher:
        raise RuntimeError(f'Refusing to overwrite an existing custom shell: {destination}')
    lockscreen = source / 'contents/lockscreen'
    main_block = replace_once((lockscreen / 'MainBlock.qml').read_text(),
                              'SessionManagementScreen {', 'SequoiaSession {')
    main_block = replace_once(main_block, '    RowLayout {\n        Layout.fillWidth: true', """    ColumnLayout {
        Layout.fillWidth: true
        spacing: 14 * sessionManager.unit""")
    main_block = replace_once(main_block,
        '            font.pointSize: Kirigami.Theme.defaultFont.pointSize + 1\n', '')
    main_block = replace_once(main_block, '            id: passwordBox\n', """            id: passwordBox
            objectName: "unlockPassword"
            font.pixelSize: Math.max(10, 12 * sessionManager.unit)
            implicitHeight: 28 * sessionManager.unit
            Layout.minimumHeight: implicitHeight
            Layout.maximumHeight: implicitHeight
            color: sessionManager.themeColors.foreground
            placeholderTextColor: sessionManager.themeColors.muted
            selectionColor: sessionManager.themeColors.accent
            leftPadding: 28 * sessionManager.unit
            topPadding: 3 * sessionManager.unit
            bottomPadding: 3 * sessionManager.unit
            background: Rectangle {
                radius: 7 * sessionManager.unit
                color: sessionManager.themeColors.field
                border.color: passwordBox.activeFocus ? sessionManager.themeColors.focusColor : sessionManager.themeColors.border
                border.width: passwordBox.activeFocus ? 1.5 : 1
            }
            Kirigami.Icon {
                x: 9 * sessionManager.unit
                anchors.verticalCenter: parent.verticalCenter
                width: 12 * sessionManager.unit; height: 15 * sessionManager.unit
                source: "lock"
                color: sessionManager.themeColors.foreground
            }
""")
    main_block = replace_once(main_block, '        PlasmaComponents3.Button {\n            id: loginButton', """        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8 * sessionManager.unit
        PlasmaComponents3.Button {
            id: loginButton
            objectName: "unlockButton"
            text: i18nd("plasma_shell_org.kde.plasma.desktop", "Unlock")
            font.pixelSize: Math.max(10, 12 * sessionManager.unit)
            leftPadding: 9 * sessionManager.unit
            rightPadding: 9 * sessionManager.unit
            topPadding: 0
            bottomPadding: 0
            icon.width: 12 * sessionManager.unit
            icon.height: 12 * sessionManager.unit
            display: QQC2.AbstractButton.TextBesideIcon
            Kirigami.Theme.textColor: "white"
            background: Rectangle {
                radius: 7 * sessionManager.unit
                color: loginButton.down ? "#1685da" : sessionManager.themeColors.accent
                border.color: loginButton.visualFocus ? sessionManager.themeColors.focusColor : sessionManager.themeColors.border
                border.width: 1.5
            }""")
    main_block = replace_once(main_block,
        '            Layout.preferredWidth: loginButton.Layout.preferredHeight',
        '            Layout.preferredWidth: implicitWidth')
    main_block = replace_once(main_block, '            Keys.onReturnPressed: clicked()\n        }', """            Keys.onReturnPressed: clicked()
        }
        PlasmaComponents3.Button {
            objectName: "switchUserButton"
            text: i18nd("plasma_shell_org.kde.plasma.desktop", "Switch User")
            icon.name: "system-switch-user"
            icon.width: 14 * sessionManager.unit
            icon.height: 14 * sessionManager.unit
            font.pixelSize: Math.max(10, 12 * sessionManager.unit)
            Layout.preferredHeight: passwordBox.implicitHeight
            leftPadding: 3 * sessionManager.unit
            rightPadding: 3 * sessionManager.unit
            topPadding: 0; bottomPadding: 0
            visible: sessionManagement.canSwitchUser
            flat: true
            onClicked: sessionManagement.switchUser()
        }
        }""")
    screen_ui = (lockscreen / 'LockScreenUi.qml').read_text()
    screen_ui = replace_once(screen_ui, '    id: lockScreenUi', """    id: lockScreenUi
    readonly property alias themeColors: appearance
    readonly property real unit: Math.max(0.65, Math.min(2, width / 1024, height / 560))
    readonly property real loginUnit: Math.max(0.75, unit * 0.8)
    readonly property real powerUnit: Math.max(0.85, unit * 0.8)
    SequoiaPalette { id: appearance }
""")
    screen_ui = replace_once(screen_ui, '    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary', """    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.textColor: appearance.foreground
    Kirigami.Theme.backgroundColor: appearance.surface
    Kirigami.Theme.highlightColor: appearance.accent""")
    screen_ui = replace_once(screen_ui, '        Clock {', '        SequoiaClock {')
    screen_ui = replace_once(screen_ui, '            anchors.horizontalCenter: parent.horizontalCenter\n            y: (mainBlock.userList.y + mainStack.y)/2 - height/2', """            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 28 * lockScreenUi.unit
            anchors.bottomMargin: 22 * lockScreenUi.unit""")
    screen_ui = replace_once(screen_ui, '        WallpaperFader {', '        SequoiaWallpaperFader {')
    screen_ui = replace_once(screen_ui, 'height: lockScreenRoot.height + Kirigami.Units.gridUnit * 3',
                             'height: lockScreenRoot.height')
    screen_ui = replace_once(screen_ui, 'showUserList: userList.y + mainStack.y > 0', 'showUserList: true')
    start = screen_ui.index('                actionItems: [')
    end = screen_ui.index('\n                Loader {', start)
    screen_ui = screen_ui[:start] + """                actionItems: [
                    SequoiaActionButton {
                        text: i18nd("plasma_shell_org.kde.plasma.desktop", "Sleep")
                        icon.name: "system-suspend"
                        enabled: sessionManagement.canSuspend
                        onClicked: sessionManagement.suspend()
                    },
                    SequoiaActionButton {
                        text: i18nd("plasma_shell_org.kde.plasma.desktop", "Restart")
                        icon.name: "system-reboot"
                        enabled: sessionManagement.canReboot
                        onClicked: sessionManagement.requestReboot()
                    },
                    SequoiaActionButton {
                        text: i18nd("plasma_shell_org.kde.plasma.desktop", "Shut Down")
                        icon.name: "system-shutdown"
                        enabled: sessionManagement.canShutdown
                        onClicked: sessionManagement.requestShutdown()
                    }
                ]
""" + screen_ui[end:]
    screen_ui = replace_once(screen_ui, '                Loader {\n                    Layout.topMargin:',
        '                Loader {\n                    parent: mainBlock.mediaArea\n                    anchors.fill: parent\n                    onLoaded: item.appearance = Qt.binding(() => appearance)\n                    Layout.topMargin:')
    screen_ui = replace_once(screen_ui, """                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: Kirigami.Units.smallSpacing""", """                top: parent.top
                right: parent.right
                topMargin: 8 * lockScreenUi.unit
                rightMargin: 18 * lockScreenUi.unit""")
    screen_ui = replace_once(screen_ui, '            spacing: Kirigami.Units.smallSpacing\n\n            PlasmaComponents3.ToolButton', """            spacing: Kirigami.Units.smallSpacing
            SequoiaGlass {
                parent: lockScreenRoot
                z: 1
                opacity: footer.opacity
                anchors.fill: footer
                anchors.leftMargin: -4 * lockScreenUi.unit
                anchors.rightMargin: -4 * lockScreenUi.unit
                anchors.topMargin: -2 * lockScreenUi.unit
                anchors.bottomMargin: -2 * lockScreenUi.unit
                radius: 9 * lockScreenUi.unit
                visible: footer.implicitWidth > 0
                sceneSource: wallpaper
                scene: lockScreenUi
                themeColors: appearance
            }
            PlasmaComponents3.ToolButton""")
    screen_ui = replace_once(screen_ui, '                id: virtualKeyboardButton', """                id: virtualKeyboardButton
                display: AbstractButton.IconOnly
                Accessible.name: text
                ToolTip.text: text
                ToolTip.visible: hovered""")
    for button_id in ('virtualKeyboardButton', 'keyboardButton'):
        screen_ui = replace_once(screen_ui, f'                id: {button_id}', f"""                id: {button_id}
                implicitHeight: Math.max(24, 22 * lockScreenUi.unit)
                Layout.minimumHeight: implicitHeight
                Layout.maximumHeight: implicitHeight
                topPadding: 2 * lockScreenUi.unit
                bottomPadding: 2 * lockScreenUi.unit
                icon.width: 16 * lockScreenUi.unit
                icon.height: 16 * lockScreenUi.unit""")
    screen_ui = replace_once(screen_ui, '            id: footer', '            id: footer\n            z: 2')
    screen_ui = replace_once(screen_ui, """            Item {
                Layout.fillWidth: true
            }

            Battery {}""", '            Battery {}')
    # Copy the complete shell so its other resources continue to resolve locally.
    # Re-running after Plasma upgrades refreshes these unmodified system files.
    shutil.copytree(source, destination, dirs_exist_ok=True)
    if keep_launcher:
        (destination / COMPACT).write_text(patch_launcher((source / COMPACT).read_text()))
    (destination / 'contents/lockscreen/MainBlock.qml').write_text(main_block)
    (destination / 'contents/lockscreen/LockScreenUi.qml').write_text(screen_ui)
    no_password = (lockscreen / 'NoPasswordUnlock.qml').read_text().replace('SessionManagementScreen {','SequoiaSession {')
    no_password = no_password.replace('PlasmaComponents3.Button {', 'PlasmaComponents3.Button {\n        objectName: "noPasswordUnlock"')
    (destination / 'contents/lockscreen/NoPasswordUnlock.qml').write_text(no_password)
    for path in (ROOT / 'lockscreen').glob('*.qml'):
        shutil.copy2(path, destination / 'contents/lockscreen' / path.name)
    shutil.copy2(ROOT / 'login-screen/qml/Avatar.qml', destination / 'contents/lockscreen/SequoiaAvatar.qml')
    marker.write_text('Local appearance override; refresh with tools/install-lockscreen.py after Plasma updates.\n')
    print(f'Installed lock-screen appearance override to {destination}')


if __name__ == '__main__':
    main()
