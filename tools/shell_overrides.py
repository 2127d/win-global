"""Compose Plasma-style (launcher/picker) and lock-screen shell overrides."""
from pathlib import Path
import shutil
import subprocess
import sys
import widget_picker

ROOT = Path(__file__).resolve().parents[1]
SHELL = 'plasma/shells/org.kde.plasma.desktop'
SYSTEM_SHELL = Path('/usr/share') / SHELL
COMPACT = 'contents/applet/CompactApplet.qml'
MARKERS = {'plasma-style': '.sequoia-eleven-launcher', 'lockscreen': '.sequoia-eleven-lockscreen'}
COMPONENTS = set(MARKERS)


def is_shell(name):
    return name.startswith(SHELL + '/')


def installed(files):
    return {component for component, marker in MARKERS.items() if SHELL + '/' + marker in files}


def patch_launcher(text):
    def replace(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise RuntimeError('The installed Plasma CompactApplet has changed; review the launcher override before installing.')
        text = text.replace(old, new, 1)

    replace('        id: expandedItem\n', '''        id: expandedItem

        // Sequoia: isolate launcher geometry from calendar and tray tab frames.
        KSvg.Svg {
            id: launcherArtwork
            imagePath: "widgets/tabbar"
            property bool available: false
            function refreshAvailability() {
                available = hasElement("sequoia-launcher-center");
            }
            Component.onCompleted: refreshAvailability()
            onRepaintNeeded: refreshAvailability()
        }
        readonly property bool squareLauncher: launcherArtwork.available
            && ["org.kde.plasma.kickoff", "org.kde.plasma.kicker", "org.kde.plasma.kickerdash"].indexOf(Plasmoid.pluginName) !== -1
        readonly property real launcherSide: Math.min(root.width, root.height)
''')
    for edge in ('bottom', 'top', 'left', 'right'):
        horizontal = edge in ('left', 'right')
        condition = 'vertical' if horizontal else '!vertical'
        dimension = 'width' if horizontal else 'height'
        old = f"{edge}Margin: {condition} && containerMargins ? -containerMargins('{edge}', returnAllMargins) : 0"
        expression = old.split(': ', 1)[1].replace('vertical', 'root.vertical').replace('containerMargins', 'expandedItem.containerMargins')
        replace(old, f'{edge}Margin: expandedItem.squareLauncher ? (root.{dimension} - expandedItem.launcherSide) / 2 : ({expression})')
    replace('        prefix: {\n            let prefix;', '''        prefix: {
            if (squareLauncher) {
                return "sequoia-launcher";
            }
            let prefix;''')
    return text


def stage(stage_root, prefix, selected, managed, *, removing=()):
    """Build the complete shell, retaining the other independently selected layer.

    Plasma does not merge files from user and system packages with the same ID.
    Keep a complete base, and rebuild it without a layer when that layer is removed.
    """
    selected, removing = set(selected), set(removing)
    current = installed(managed)
    wanted = (current | (selected & COMPONENTS)) - removing
    if not wanted:
        return
    source = Path(prefix) / SHELL
    if source.exists() and not any((source / marker).is_file() for marker in MARKERS.values()):
        raise RuntimeError(f'Existing custom Plasma shell must be preserved: {source}')
    destination = Path(stage_root) / SHELL
    if 'lockscreen' in selected and 'lockscreen' not in removing:
        subprocess.run([sys.executable, str(ROOT / 'tools/install-lockscreen.py'), '--prefix', str(stage_root)], check=True)
    else:
        shutil.copytree(SYSTEM_SHELL, destination, dirs_exist_ok=True)
        if 'lockscreen' in wanted:
            shutil.rmtree(destination / 'contents/lockscreen')
            shutil.copytree(source / 'contents/lockscreen', destination / 'contents/lockscreen')
            shutil.copy2(source / MARKERS['lockscreen'], destination / MARKERS['lockscreen'])
    if 'plasma-style' in wanted:
        target = destination / COMPACT
        if 'plasma-style' in selected:
            target.write_text(patch_launcher((SYSTEM_SHELL / COMPACT).read_text()))
            widget_picker.stage(destination, SYSTEM_SHELL)
        else:
            shutil.copy2(source / COMPACT, target)
            for name in widget_picker.FILES:
                if (source / name).exists():
                    shutil.copy2(source / name, destination / name)
        (destination / MARKERS['plasma-style']).write_text('Launcher geometry and centered widget picker; refresh through the Sequoia installer after Plasma updates.\n')


def scope(components):
    """Whether a selection operates on the composed shell package."""
    return bool(set(components) & COMPONENTS)
