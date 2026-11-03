# SPDX-License-Identifier: MIT
"""Display spotlight, using Plasma brightness, theme, and Night Light controls."""
import shutil


def style_display(destination, source, replace):
    directory = destination / 'brightness'
    for path in (source / 'display').iterdir():
        if path.suffix in ('.qml', '.js') and path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: brightnessAndColorControl\n',
                   '    id: brightnessAndColorControl\n    readonly property bool sequoiaFooterActions: true\n')
    text = replace(text, 'Plasmoid.title: i18n("Brightness & Color")',
                   'Plasmoid.title: i18n("Display")')
    text = replace(text, 'Layout.preferredWidth: Kirigami.Units.gridUnit * 20',
                   'Layout.preferredWidth: 418')
    path.write_text(text)
