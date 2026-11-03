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
    path.write_text(text)
