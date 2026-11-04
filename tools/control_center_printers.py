# SPDX-License-Identifier: MIT
"""Keep KDE's printer/job models and restyle their presentation."""
import shutil


def style_printers(destination, source, replace):
    directory = destination / 'printmanager'
    for path in (source / 'printers').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '    id: root\n    readonly property bool sequoiaFooterActions: true\n')
    text = replace(text, '        onRowsRemoved: root.printerCount = rowCount()', '''        onRowsRemoved: root.printerCount = rowCount()
        onModelReset: root.printerCount = rowCount()
        Component.onCompleted: root.printerCount = rowCount()''')
    path.write_text(text)
