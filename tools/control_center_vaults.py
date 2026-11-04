# SPDX-License-Identifier: MIT
"""Restyle the external Vault QML package while preserving its native model."""
from pathlib import Path
import shutil


def style_vaults(destination, source, plugin_dir, replace):
    upstream = Path('/usr/share/plasma/plasmoids/org.kde.plasma.vault')
    directory = destination / 'vault'
    shutil.copytree(upstream / 'contents/ui', directory)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '    id: root\n    readonly property bool sequoiaFooterActions: true\n')
    start = text.index('    fullRepresentation: PlasmaExtras.Representation {')
    text = text[:start] + '''    fullRepresentation: FullRepresentation {
        vaultsModel: Plasmoid.vaultsModel
        actions: root.vaultsModelActions
    }
}
'''
    path.write_text(text)
    for path in (source / 'vaults').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    return [upstream / 'metadata.json', *sorted((upstream / 'contents/ui').glob('*.qml')),
            plugin_dir / 'org.kde.plasma.vault.so']
