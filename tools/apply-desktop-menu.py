#!/usr/bin/env python3
"""Restore the compact standard desktop context menu shown in the reference."""
import argparse
import configparser
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


OPTIONS = {
    '_context': True,
    '_sep1': True,
    'configure shortcuts': False,
    '_open_terminal': True,
    '_run_command': False,
    'add widgets': False,
    '_add panel': False,
    'manage activities': False,
    'remove': False,
    'desktop edit mode': True,
    'edit mode': True,
    '_sep2': True,
    '_lock_screen': True,
    '_logout': True,
    '_sep3': True,
    '_wallpaper': False,
    '_sep4': False,
    'configure': True,
    '_display_settings': True,
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--restart-shell', action='store_true',
                        help='Reload the desktop and panels through the Plasma user service.')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    # Desktop (0) only. Panel context menus (1) retain their existing settings.
    script = '''
var actions = ConfigFile("plasma-org.kde.plasma.desktop-appletsrc", "ActionPlugins");
var desktop = ConfigFile(actions, "0");
desktop.writeEntry("RightButton;NoModifier", "org.kde.contextmenu");
var menu = ConfigFile(desktop, "RightButton;NoModifier");
var options = OPTIONS;
for (var key in options) {
    if (!menu.writeEntry(key, options[key])) throw new Error("Cannot write " + key);
}
for (var key in options) {
    if (String(menu.readEntry(key)) !== String(options[key])) throw new Error("Cannot verify " + key);
}
print("Desktop menu settings verified.");
'''.replace('OPTIONS', json.dumps(OPTIONS))
    if args.dry_run:
        print(script)
        return
    if args.restart_shell:
        subprocess.run(['systemctl', '--user', 'is-active', '--quiet', 'plasma-plasmashell.service'], check=True)
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
    backup = data / 'sequoia-eleven/backups' / ('desktop-menu-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    shutil.copy2(config / 'plasma-org.kde.plasma.desktop-appletsrc', backup)
    print('Previous desktop configuration:', backup, flush=True)
    # Write through the running shell's KConfig instance to avoid a stale
    # in-memory configuration overwriting edits made directly on disk.
    subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                    'org.kde.PlasmaShell.evaluateScript', script], check=True)
    # ConfigFile batches writes on a timer. Confirm they reached disk before
    # restarting the shell (sync() is not exposed by the scripting API).
    for attempt in range(50):
        saved = configparser.ConfigParser(interpolation=None)
        saved.read(config / 'plasma-org.kde.plasma.desktop-appletsrc')
        section = 'ActionPlugins][0][RightButton;NoModifier'
        if saved.has_section(section) and all(
                saved.get(section, key, fallback='').lower() == str(value).lower()
                for key, value in OPTIONS.items()):
            break
        time.sleep(0.1)
    else:
        raise RuntimeError('Plasma has not saved the menu settings; the shell was not restarted.')
    if args.restart_shell:
        subprocess.run(['systemctl', '--user', 'restart', 'plasma-plasmashell.service'], check=True)
        subprocess.run(['systemctl', '--user', 'is-active', 'plasma-plasmashell.service'], check=True)
    else:
        print('The menu takes effect when Plasma next starts.')


if __name__ == '__main__':
    main()
