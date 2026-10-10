#!/usr/bin/env python3
"""Activate an installed desktop theme, with a snapshot of prior settings."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from typography import FONT_SETTINGS
from wallpaper import WALLPAPER_ID

ROOT = Path(__file__).resolve().parents[1]
HOME_DIR = Path.home()
DATA = Path(os.environ.get('XDG_DATA_HOME') or HOME_DIR/'.local/share')
CONFIG = Path(os.environ.get('XDG_CONFIG_HOME') or HOME_DIR/'.config')


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def write(file, group, key, value):
    run('kwriteconfig6', '--file', CONFIG/file, '--group', group, '--key', key, value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=['dark','light'], default='dark')
    parser.add_argument('--material', choices=['solid', 'transparent', 'legacy'], default='solid')
    parser.add_argument('--keep-wallpaper', action='store_true',
                        help='Keep the current desktop background when selecting the theme.')
    args = parser.parse_args()
    suffix = '' if args.material == 'legacy' else '.' + args.material
    scheme_suffix = '' if args.material == 'legacy' else args.material.title()
    if not Path('/usr/lib64/qt6/plugins/styles/sequoiaeleven6.so').exists():
        raise RuntimeError('Install the compiled plugins first; see README.md.')
    if not Path(f'/usr/lib64/qt6/plugins/org.kde.kdecoration3/org.kde.sequoiaeleven{suffix}.so').exists():
        raise RuntimeError('Install the matching compiled window decoration first; see README.md.')
    paths = [CONFIG/name for name in ['kdeglobals','kwinrc','plasmarc','ksplashrc','kscreenlockerrc','sequoiaelevenrc']]
    required = [DATA/'plasma/desktoptheme'/f'sequoia-eleven-{args.mode}',
                DATA/'plasma/look-and-feel'/f'org.sequoia.eleven.{args.mode}{suffix}',
                DATA/'color-schemes'/f'SequoiaEleven{args.mode.title()}{scheme_suffix}.colors',
                DATA/'icons'/f'SequoiaTahoe-{args.mode}',
                DATA/'fonts/sequoia-eleven/AdwaitaSans', DATA/'wallpapers'/WALLPAPER_ID]
    if not all(path.exists() for path in required):
        raise RuntimeError('Install the theme first with ./install.sh.')
    if not args.keep_wallpaper:
        paths += [CONFIG/'plasma-org.kde.plasma.desktop-appletsrc']
    backup = DATA/'sequoia-eleven/backups'/datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True)
    manifest=[]
    for index,path in enumerate(paths):
        entry={'path':str(path),'exists':path.exists(),'directory':path.is_dir(),'copy':str(index)}
        if path.is_dir(): shutil.copytree(path,backup/str(index))
        elif path.exists(): shutil.copy2(path,backup/str(index))
        manifest.append(entry)
    (backup/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Previous settings saved to',backup,flush=True)
    write('kdeglobals','KDE','widgetStyle','SequoiaEleven')
    write('kdeglobals','Icons','Theme',f'SequoiaTahoe-{args.mode}')
    write('kdeglobals','KDE','LookAndFeelPackage',f'org.sequoia.eleven.{args.mode}{suffix}')
    for group, settings in FONT_SETTINGS.items():
        for key, value in settings.items():
            write('kdeglobals', group, key, value)
    write('kwinrc','org.kde.kdecoration2','library','org.kde.sequoiaeleven' + suffix)
    write('kwinrc','org.kde.kdecoration2','theme','')
    write('kwinrc','org.kde.kdecoration2','ButtonsOnLeft','XIA')
    write('kwinrc','org.kde.kdecoration2','ButtonsOnRight','')
    write('kwinrc','org.kde.kdecoration2','BorderSize','None')
    write('kwinrc','Plugins','blurEnabled','true')
    write('ksplashrc','KSplash','Theme',f'org.sequoia.eleven.{args.mode}')
    write('ksplashrc','KSplash','Engine','KSplashQML')
    run('plasma-apply-colorscheme',f'SequoiaEleven{args.mode.title()}{scheme_suffix}')
    run('plasma-apply-desktoptheme',f'sequoia-eleven-{args.mode}')
    if not args.keep_wallpaper:
        # Select the package, without a #light/#dark fragment, to follow colors.
        run('plasma-apply-wallpaperimage', DATA/'wallpapers'/WALLPAPER_ID)
    run('qdbus-qt6','org.kde.KWin','/KWin','org.kde.KWin.reconfigure')
    run('dbus-send','--session','--type=signal','/KGlobalSettings','org.kde.KGlobalSettings.notifyChange','int32:0','int32:0')
    run('dbus-send','--session','--type=signal','/KGlobalSettings','org.kde.KGlobalSettings.notifyChange','int32:2','int32:0')
    run('dbus-send','--session','--type=signal','/KIconLoader','org.kde.KIconLoader.iconChanged','int32:0')
    print(f'Applied Sequoia Eleven {args.mode.title()} {args.material.title()}. Restart open applications to load the new style.')
    print(f'Restore with: python3 tools/restore-desktop.py {backup}')


if __name__ == '__main__':
    main()
