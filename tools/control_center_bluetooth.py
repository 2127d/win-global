# SPDX-License-Identifier: MIT
"""Bluetooth spotlight presentation, retaining KDE's device model and actions."""
import shutil


def style_bluetooth(destination, source, replace):
    directory = destination / 'bluetooth'
    for path in (source / 'bluetooth').iterdir():
        if path.suffix in ('.qml', '.js') and path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), 'import QtQuick\n',
                   'import QtQuick\nimport org.kde.plasma.components as PC\n')
    text = replace(text, '    id: root\n', '''    id: root
    Plasmoid.title: i18n("Bluetooth")
    readonly property bool sequoiaFooterActions: true
    readonly property BluetoothActivity sequoiaActivity: BluetoothActivity { monitoring: root.expanded }
    readonly property var backAction: Kirigami.Action {
        enabled: (root.fullRepresentationItem?.navigationDepth ?? 1) > 1
        onTriggered: root.fullRepresentationItem.goBack()
    }
    property Component sequoiaHeaderControl: Component {
        PC.Switch {
            objectName: "bluetoothHeaderSwitch"
            Accessible.name: i18n("Bluetooth")
            checked: BluezQt.Manager.bluetoothOperational
            enabled: root.toggleBluetoothAction.visible
            onToggled: root.setBluetoothEnabled(checked)
        }
    }
''')
    text = replace(text, 'i18nc("@item:inmenu", "Show badge with number of connected devices")',
                   'i18nc("@item:inmenu", "Show connected device count")')
    text = replace(text, 'onTriggered: checked => KCMUtils.KCMLauncher.openSystemSettings("kcm_bluetooth")', '''onTriggered: {
            // Give the same module the full window on screens too narrow for the settings sidebar.
            if (root.Screen.width > 0 && root.Screen.width < Kirigami.Units.gridUnit * 36)
                KCMUtils.KCMLauncher.open(["kcm_bluetooth"]);
            else
                KCMUtils.KCMLauncher.openSystemSettings("kcm_bluetooth");
        }''')
    path.write_text(text)

    path = directory / 'Toolbar.qml'
    text = replace(path.read_text(), '''            checked: configMenu.status !== PlasmaExtras.Menu.Closed
            onToggled: checked ? configMenu.openRelative() : configMenu.close()

            PlasmaExtras.Menu {
                id: configMenu
                visualParent: moreActionsButton
                placement: PlasmaExtras.Menu.BottomPosedLeftAlignedPopup

                PlasmaExtras.MenuItem {
                    action: toggleBadgeAction
                }
            }''', '''            checked: configMenu.visible
            onToggled: checked ? configMenu.openFor(moreActionsButton) : configMenu.close()

            Details.DetailActionMenu {
                id: configMenu
                nativeActions: [root.toggleBadgeAction]
            }''')
    path.write_text(text)

    path = directory / 'MediaPlayerItem.qml'
    text = replace(path.read_text(), '    RowLayout {\n        spacing: 0', '''    component MediaButton: BluetoothActionButton {
        flat: true
        display: PlasmaComponents3.AbstractButton.IconOnly
        PlasmaComponents3.ToolTip { text: parent.text }
    }
    RowLayout {
        enabled: root.mediaPlayer !== null
        spacing: 4''')
    for button, label in (
        ('previousButton', 'qsTr("Previous track")'),
        ('playPauseButton', 'root.mediaPlayer?.status === BluezQt.MediaPlayer.Playing ? qsTr("Pause") : qsTr("Play")'),
        ('stopButton', 'qsTr("Stop")'),
        ('nextButton', 'qsTr("Next track")'),
    ):
        text = replace(text, f'Details.DetailToolButton {{\n            id: {button}',
                       f'MediaButton {{\n            id: {button}\n            text: {label}')
    path.write_text(text)
