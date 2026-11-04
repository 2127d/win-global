# SPDX-License-Identifier: MIT
"""Restyle the QML KDE Connect applet, retaining installed KDE Connect plugins."""
from pathlib import Path
import shutil


def style_kdeconnect(destination, source, replace):
    upstream = Path('/usr/share/plasma/plasmoids/org.kde.kdeconnect')
    if not upstream.is_dir():
        return []
    directory = destination / 'kdeconnect'
    shutil.copytree(upstream / 'contents/ui', directory)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '''    id: root
    readonly property bool sequoiaFooterActions: true
    Plasmoid.title: i18n("KDE Connect")
''')
    path.write_text(text)
    for path in (source / 'kdeconnect').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    return [upstream / 'metadata.json', *sorted((upstream / 'contents/ui').glob('*.qml')),
            Path('/usr/lib64/qt6/qml/org/kde/kdeconnect/libkdeconnectdeclarativeplugin.so')]
