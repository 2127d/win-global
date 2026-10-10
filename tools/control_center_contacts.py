# SPDX-License-Identifier: MIT
"""Style Merkuro's contacts applet while retaining its Akonadi backend."""
import shutil


def style_contacts(destination, source, plugin_dir, extract, replace):
    plugin = plugin_dir / 'org.kde.merkuro.contact.applet.so'
    if not plugin.exists():
        return []
    directory = destination / 'contacts'
    extract(plugin, ':/qt/qml/plasma/applet/org/kde/merkuro/contact/applet/', directory)
    for path in (source / 'contacts').glob('*.qml'):
        shutil.copy2(path, directory / path.name)
    return [plugin]
