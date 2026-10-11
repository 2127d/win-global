#!/usr/bin/env python3
"""Install the desktop menu and add it to existing global-menu panels, with backup."""
import argparse
from datetime import datetime
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.sequoia.desktopmenu'
SETUP = '''
var targets = panels().filter(p => p.widgets().some(w => w.type === "org.kde.plasma.appmenu"));
if (!targets.length) throw new Error("No panel with a global menu found");
targets.forEach(p => {
    var menu = p.widgets().find(w => w.type === "org.sequoia.desktopmenu")
        || p.addWidget("org.sequoia.desktopmenu");
    if (!menu) throw new Error("Could not create desktop menu");
    var appmenu = p.widgets().find(w => w.type === "org.kde.plasma.appmenu");
    // A just-created applet can report -1 until the panel inserts it.
    menu.index = menu.index >= 0 && menu.index < appmenu.index ? appmenu.index - 1 : appmenu.index;
    var title = p.widgets().find(w => w.type === "org.kde.windowtitle.Fork");
    if (title) {
        title.currentConfigGroup = ["General"];
        title.writeConfig("customText", true);
        title.writeConfig("textDefault", "");
        title.writeConfig("placeHolder", "");
        title.writeConfig("placeHolderIcon", "");
        title.reloadConfig();
    }
});
print("Desktop menu installed in " + targets.length + " panel(s).");
'''


def evaluate(script):
    return subprocess.check_output(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
        'org.kde.PlasmaShell.evaluateScript', script], text=True, timeout=30).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, help='Copy the widget to a staging data tree only')
    parser.add_argument('--update-title', action='store_true',
                        help='Apply the minimized-window fix to the installed Window Title Fork, with backup')
    parser.add_argument('--restart', action='store_true', help='Reload Plasma after updating existing widget code')
    args = parser.parse_args()
    source = ROOT / 'plasma/plasmoids' / PLUGIN
    if args.stage:
        shutil.copytree(source, args.stage / 'plasma/plasmoids' / PLUGIN, dirs_exist_ok=True)
        return
    # Fail before changing files if the current shell has no suitable panel.
    count = evaluate('print(panels().filter(p => p.widgets().some(w => w.type === "org.kde.plasma.appmenu")).length);')
    if not count.isdigit() or int(count) == 0:
        raise RuntimeError('No panel with a global menu found: ' + count)
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    target = data / 'plasma/plasmoids' / PLUGIN
    backup = data / 'sequoia-eleven/backups' / ('desktop-menubar-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    shutil.copy2(config / 'plasma-org.kde.plasma.desktop-appletsrc', backup)
    # Also retain a live layout snapshot, including any not-yet-flushed changes.
    snapshot = subprocess.check_output(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
        'org.kde.PlasmaShell.dumpCurrentLayoutJS'], text=True, timeout=30)
    (backup / 'layout.js').write_text(snapshot)
    if target.exists():
        shutil.copytree(target, backup / PLUGIN)
    if args.update_title:
        title_id = 'org.kde.windowtitle.Fork'
        title_target = data / 'plasma/plasmoids' / title_id
        if not (title_target / 'metadata.json').is_file():
            raise RuntimeError('No per-user Window Title Fork found; no widget files were changed.')
        for name in ('main.qml', 'PlasmaTasksModel.qml', 'TitleLayout.qml'):
            relative = Path('contents/ui') / name
            saved = backup / title_id / relative
            saved.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(title_target / relative, saved)
        for name in ('main.qml', 'PlasmaTasksModel.qml', 'TitleLayout.qml'):
            relative = Path('contents/ui') / name
            shutil.copy2(ROOT / 'plasma/plasmoids' / title_id / relative, title_target / relative)
    subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet',
                    '--upgrade' if target.exists() else '--install', str(source)], check=True)
    output = evaluate(SETUP)
    if not output.startswith('Desktop menu installed in '):
        raise RuntimeError(output + '\nBackup: ' + str(backup))
    print(output)
    print('Backup:', backup)
    if args.restart:
        subprocess.run(['systemctl', '--user', 'restart', 'plasma-plasmashell.service'], check=True)


if __name__ == '__main__':
    main()
