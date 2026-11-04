# SPDX-License-Identifier: MIT
"""Dwell click selection using the installed KWin D-Bus interface."""
import shutil


def style_dwell(destination, source, plugin_dir, extract, replace):
    plugin = plugin_dir / 'org.kde.plasma.dwellclicker.so'
    directory = destination / 'dwellclicker'
    extract(plugin, ':/qt/qml/plasma/applet/org/kde/plasma/dwellclicker/', directory)
    path = directory / 'main.qml'
    text = path.read_text()
    text = replace(text, 'import org.kde.kirigami as Kirigami', 'import org.kde.kirigami as Kirigami\nimport org.kde.kcmutils\nimport org.kde.config')
    text = replace(text, '    id: root\n', '    id: root\n    readonly property bool sequoiaFooterActions: true\n')
    text = replace(text, 'property int button: Number(properties.button)', 'property int button: Number(properties.button ?? 0)')
    start = text.index('    fullRepresentation: PlasmaExtras.Representation {')
    text = text[:start] + '''    QtObject {
        id: nativeController
        readonly property bool available: dbusServiceWatcher.registered
        readonly property int selected: root.selectedButtonId
        readonly property bool settingsAllowed: KAuthorized.authorizeControlModule("kcm_access")
        function select(button) {
            if (!available || ![10, 11, 12, 13, 14].includes(button)) return;
            const target = selected === button ? 0 : button;
            if (root.hideOnWindowDeactivate) root.expanded = false;
            dbusProperties.properties.button = target;
        }
        function settings() { if (settingsAllowed) KCMLauncher.openSystemSettings("kcm_access"); }
    }
    fullRepresentation: FullRepresentation { controller: nativeController }
}
'''
    path.write_text(text)
    for path in (source / 'dwell').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    return [plugin]
