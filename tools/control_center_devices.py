# SPDX-License-Identifier: MIT
"""Disks & Devices presentation retaining the native filter and action models."""
import shutil


def style_devices(destination, source, replace):
    directory = destination / 'devicenotifier'
    for path in (source / 'devices').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: devicenotifier\n', '''    id: devicenotifier
    readonly property bool sequoiaFooterActions: true
    function showOperationFeedback(success) {
        popupIcon = success ? "dialog-ok" : "dialog-error";
        popupIconTimer.restart();
    }
''')
    path.write_text(text)
