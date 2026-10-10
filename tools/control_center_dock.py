# SPDX-License-Identifier: MIT
"""Version-checked dock motion and floating configuration surfaces."""
import re
import shutil
import subprocess
from pathlib import Path
from xml.sax.saxutils import escape


def style_dock(destination, source, plugin_dir, extract, replace):
    plugin = plugin_dir / 'org.kde.plasma.taskmanager.so'
    directory = destination / 'taskmanager'
    extract(plugin, ':/qt/qml/plasma/applet/org/kde/plasma/taskmanager/', directory)
    path = directory / 'Task.qml'
    text = path.read_text()
    text = replace(text, '    id: task\n', '''    id: task
    // Confine dock styling to horizontal icons-only tasks, never popup lists.
    readonly property bool sequoiaDock: tasksRoot.iconsOnly && !tasksRoot.vertical && !inPopup
    readonly property bool sequoiaMotion: sequoiaDock && Plasmoid.configuration.dockAnimations && Kirigami.Units.longDuration > 0
    readonly property int sequoiaDuration: sequoiaMotion ? Math.round(Kirigami.Units.longDuration * 100 / Math.max(50, Plasmoid.configuration.dockSpeed)) : 0
    objectName: sequoiaDock ? "sequoiaDockTask" : ""
''')
    text = replace(text, '        id: frame\n', '''        id: frame
        // Keep the native frame and its drag handler alive, but replace its paint.
        opacity: task.sequoiaDock ? 0 : 1
''')
    text = replace(text, '    Loader {\n        id: taskProgressOverlayLoader', '''    Rectangle {
        id: runningDot
        objectName: "sequoiaRunningDot"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: 4
        height: 4
        radius: 2
        visible: task.sequoiaDock && (task.model.IsWindow || task.model.IsGroupParent)
        color: task.model.IsDemandingAttention ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
        property real pulse: 1
        opacity: (task.model.IsActive ? 0.95 : 0.55) * pulse
        SequentialAnimation on pulse {
            running: task.sequoiaMotion && Plasmoid.configuration.dockAttentionAnimation && task.model.IsDemandingAttention
            loops: Animation.Infinite
            onStopped: runningDot.pulse = 1
            NumberAnimation { to: 0.35; duration: task.sequoiaDuration * 2; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1; duration: task.sequoiaDuration * 2; easing.type: Easing.InOutSine }
        }
    }

    Loader {
        id: taskProgressOverlayLoader''')
    text = replace(text, '        id: iconBox\n', '''        id: iconBox
        // Scale within the padded cell; click and drag target geometry stays fixed.
        objectName: "sequoiaDockIcon"
        scale: dockMotion.zoom
        transformOrigin: Item.Center
        transform: Translate { y: dockMotion.offset }
        DockMotion {
            id: dockMotion
            animationsEnabled: task.sequoiaMotion
            hovered: task.highlighted && Plasmoid.configuration.taskHoverEffect
            pressed: leftTapHandler.pressed
            launching: task.model.IsStartup
            launchEnabled: Plasmoid.configuration.dockLaunchAnimation ?? true
            pressEnabled: Plasmoid.configuration.dockPressAnimation ?? true
            hoverStyle: Plasmoid.configuration.dockHoverStyle ?? 2
            strength: Plasmoid.configuration.dockStrength ?? 12
            speed: Plasmoid.configuration.dockSpeed ?? 100
            direction: Plasmoid.location === PlasmaCore.Types.TopEdge ? 1 : -1
        }
''')
    text = text.replace('duration: Kirigami.Units.longDuration',
        'duration: task.sequoiaDock ? (Plasmoid.configuration.dockReorderAnimation ? task.sequoiaDuration : 0) : Kirigami.Units.longDuration')
    path.write_text(text)
    style_settings(directory, source, replace)
    # Keep KDE's Trash widget and its native drop/open/empty actions. The panel
    # normally forces symbolic artwork; use the active icon theme's regular
    # empty/full bins so Sequoia Tahoe supplies its light/dark variants.
    trash_plugin = plugin_dir / 'org.kde.plasma.trash.so'
    trash = destination / 'trash'
    extract(trash_plugin, ':/qt/qml/plasma/applet/org/kde/plasma/trash/', trash)
    path = trash / 'main.qml'
    text = replace(path.read_text(), '''        if (inPanel) {
            return iconName += "-symbolic";
        }

''', '')
    path.write_text(text)
    return [plugin, trash_plugin]


def style_settings(directory, source, replace):
    for path in (source / 'taskmanager').glob('*.qml'):
        shutil.copy2(path, directory / path.name)
    entries = {
        'dockAnimations': ('Bool', 'true'),
        'dockHoverStyle': ('Int', '2', 0, 2),
        'dockStrength': ('Int', '12', 0, 20),
        'dockSpeed': ('Int', '100', 50, 200),
        'dockLaunchAnimation': ('Bool', 'true'),
        'dockPressAnimation': ('Bool', 'true'),
        'dockAttentionAnimation': ('Bool', 'true'),
        'dockReorderAnimation': ('Bool', 'true'),
    }
    xml = ''
    for name, entry in entries.items():
        xml += f'    <entry name="{name}" type="{entry[0]}"><default>{entry[1]}</default>'
        if len(entry) > 2:
            xml += f'<min>{entry[2]}</min><max>{entry[3]}</max>'
        xml += '</entry>\n'
    path = directory / 'main.xml'
    path.write_text(replace(path.read_text(), '  </group>', xml + '  </group>'))
    path = directory / 'config.qml'
    path.write_text(replace(path.read_text(), 'ConfigModel {', '''ConfigModel {
    ConfigCategory {
        name: i18n("Animations")
        icon: "preferences-desktop-effects"
        source: "ConfigAnimations.qml"
    }'''))
    # Preserve KDE's controls, property aliases, accessibility and dependencies.
    # Split only the native section spacers, with explicit anchor checks.
    titles = {
        'ConfigAppearance.qml': ['Previews & indicators', 'Task labels', 'Panel layout', 'Icon spacing'],
        'ConfigBehavior.qml': ['Window grouping', 'Group options', 'Task order', 'Click actions', 'Scrolling', 'Window filters', 'Attention', 'New tasks'],
    }
    for filename, headings in titles.items():
        path = directory / filename
        text = replace(path.read_text(), '    id: root', f'    id: root\n    objectName: "sequoia{filename[:-4]}"')
        if 'import QtQuick.Layouts' not in text:
            text = text.replace('import QtQuick\n', 'import QtQuick\nimport QtQuick.Layouts\n')
        start = text.index('    Kirigami.FormLayout {')
        content = text[start + len('    Kirigami.FormLayout {'):text.rfind('    }')]
        content = content.replace('        anchors.left: parent.left\n        anchors.right: parent.right\n', '')
        sections = re.split(r'        Item \{\n            Kirigami.FormData.isSection: true\n(?:            visible: [^\n]+\n)?        \}\n', content)
        if len(sections) != len(headings):
            raise RuntimeError(f'{filename}: native settings sections changed')
        cards = '    ColumnLayout {\n        spacing: Kirigami.Units.largeSpacing\n'
        for title, section in zip(headings, sections):
            cards += f'        SettingsCard {{\n            title: i18n("{title}")\n'
            if title in ('Task labels', 'Group options'):
                cards += '            visible: Plasmoid.pluginName !== "org.kde.plasma.icontasks"\n'
            cards += '            Kirigami.FormLayout {\n                Layout.fillWidth: true\n' + section + '\n            }\n        }\n'
        path.write_text(text[:start] + cards + '    }\n}\n')
    # A private Qt resource namespace lets native config loading use the extended
    # schema and local pages as well as the task entry point.
    qrc = directory.parent / 'taskmanager.qrc'
    qrc.write_text('<RCC><qresource prefix="/sequoia/taskmanager">' + ''.join(
        f'<file alias="{escape(p.name)}">taskmanager/{escape(p.name)}</file>'
        for p in sorted(directory.iterdir()) if p.is_file()) + '</qresource></RCC>')
    from PySide6.QtCore import QLibraryInfo
    rcc = str(Path(QLibraryInfo.path(QLibraryInfo.LibraryExecutablesPath)) / 'rcc')
    subprocess.run([rcc, '--binary', str(qrc), '-o', str(directory.parent / 'taskmanager.rcc')], check=True)
