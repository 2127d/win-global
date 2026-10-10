#!/usr/bin/env python3
"""Apply verified UI fonts and conservative smoothing with a restorable snapshot."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from typography import FONT_FAMILY, FONT_SETTINGS

ROOT = Path(__file__).resolve().parents[1]
RENDER_SETTINGS = {'XftAntialias': 'true', 'XftHintStyle': 'hintslight', 'XftSubPixel': 'none'}
FONTCONFIG = '''<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <!-- Only the UI family: no aliases, symbol substitutions, or monospace changes. -->
  <match target="font">
    <test name="family" compare="eq"><string>Adwaita Sans</string></test>
    <edit name="antialias" mode="assign"><bool>true</bool></edit>
    <edit name="hinting" mode="assign"><bool>true</bool></edit>
    <edit name="hintstyle" mode="assign"><const>hintslight</const></edit>
    <edit name="rgba" mode="assign"><const>none</const></edit>
  </match>
</fontconfig>
'''


def write(path, groups, key, value):
    command = ['kwriteconfig6', '--file', str(path)]
    for group in groups:
        command += ['--group', group]
    subprocess.run(command + ['--key', key, str(value)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config-prefix', type=Path, default=Path(os.environ.get('XDG_CONFIG_HOME') or Path.home()/'.config'))
    parser.add_argument('--data-prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share'))
    parser.add_argument('--no-notify', action='store_true', help='Stage settings without notifying the live desktop')
    args = parser.parse_args()
    config, data = args.config_prefix.resolve(), args.data_prefix.resolve()
    themes = [data/'plasma/look-and-feel'/f'org.sequoia.eleven.{mode}'/'contents/defaults' for mode in ('light', 'dark')]
    themes = [path for path in themes if path.exists()]
    paths = [config/'kdeglobals', config/'gtk-3.0/settings.ini', config/'gtk-4.0/settings.ini',
             config/'fontconfig/conf.d/99-sequoia-ui-fonts.conf',
             data/'fonts/sequoia-eleven/AdwaitaSans', data/'fonts/sequoia-eleven/Nunito', *themes]
    backup = data/'sequoia-eleven/backups'/datetime.now().strftime('fonts-%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True)
    manifest = []
    for index, path in enumerate(paths):
        entry = {'path': str(path), 'exists': path.exists(), 'directory': path.is_dir(), 'copy': str(index)}
        if path.is_dir():
            shutil.copytree(path, backup/str(index))
        elif path.exists():
            shutil.copy2(path, backup/str(index))
        manifest.append(entry)
    (backup/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Font snapshot:', backup, flush=True)
    subprocess.run(['python3', str(ROOT/'tools/install-fonts.py'), '--prefix', str(data)], check=True)
    matched = subprocess.check_output(['fc-match', FONT_FAMILY, '-f', '%{family}'], text=True)
    if FONT_FAMILY not in matched.split(','):
        raise RuntimeError(f'Font did not resolve correctly ({matched}); font settings were not changed')
    for group, settings in FONT_SETTINGS.items():
        for key, value in settings.items():
            write(config/'kdeglobals', [group], key, value)
            for theme in themes:
                write(theme, ['kdeglobals', group], key, value)
    for key, value in RENDER_SETTINGS.items():
        write(config/'kdeglobals', ['General'], key, value)
    for version in ('3.0', '4.0'):
        path = config/f'gtk-{version}/settings.ini'
        path.parent.mkdir(parents=True, exist_ok=True)
        for key, value in {'gtk-font-name': f'{FONT_FAMILY} 10', 'gtk-xft-antialias': '1',
                           'gtk-xft-hinting': '1', 'gtk-xft-hintstyle': 'hintslight', 'gtk-xft-rgba': 'none'}.items():
            write(path, ['Settings'], key, value)
    rule = config/'fontconfig/conf.d/99-sequoia-ui-fonts.conf'
    rule.parent.mkdir(parents=True, exist_ok=True)
    rule.write_text(FONTCONFIG)
    if not args.no_notify:
        for change in (0, 2):
            subprocess.run(['dbus-send', '--session', '--type=signal', '/KGlobalSettings',
                            'org.kde.KGlobalSettings.notifyChange', f'int32:{change}', 'int32:0'], check=True)
        subprocess.run(['qdbus-qt6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure'], check=True)
    print('Applied Adwaita Sans 10pt, 9pt small labels, and 10pt medium window titles.')
    print(f'Restore: python3 tools/restore-desktop.py {backup}')


if __name__ == '__main__':
    main()
