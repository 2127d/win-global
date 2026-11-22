#!/usr/bin/env python3
"""Install or control the optional Jungle Vines KWin script (disabled by default)."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from theme_files import Receipt

ROOT = Path(__file__).resolve().parents[1]
ID = 'sequoia-vines-v9'
OLD_SCRIPT_ID = 'sequoia-vines'
LEGACY_ID = 'sequoia-vines-v7'


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=60).strip()


def script(method, *args):
    return run('qdbus-qt6', 'org.kde.KWin', '/Scripting', 'org.kde.kwin.Scripting.' + method, *args)


def enabled(value, name=ID):
    args = ['kwriteconfig6', '--file', 'kwinrc', '--group', 'Plugins', '--key', name + 'Enabled']
    run(*args, *(['--delete'] if value == '' else [value]))


def setting(name=ID):
    return run('kreadconfig6', '--file', 'kwinrc', '--group', 'Plugins', '--key', name + 'Enabled')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('install', 'remove', 'status', 'enable', 'disable'), nargs='?', default='install')
    args = parser.parse_args()
    if args.action == 'status':
        print('Jungle Vines enabled:', setting() or 'false (default)')
        print('Jungle Vines loaded:', script('isScriptLoaded', ID))
        return
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').absolute()
    receipt = Receipt(data, 'sequoia-eleven/vines-script-v9-installation', lambda name: name.startswith('kwin/scripts/' + ID + '/'))
    if args.action in ('enable', 'disable'):
        if args.action == 'enable' and not (data / 'kwin/scripts' / ID / 'metadata.json').exists():
            raise RuntimeError('Install Jungle Vines before enabling it.')
        enabled('true' if args.action == 'enable' else 'false')
        script('start')
        print('Jungle Vines loaded:', script('isScriptLoaded', ID))
        return
    if args.action == 'remove':
        if not receipt.manifest.exists():
            print('No tracked Jungle Vines script installation.')
            return
        receipt.validate()
        enabled('false')
        script('unloadScript', ID)
        receipt.uninstall()
        print('Removed Jungle Vines. It remains disabled.')
        return
    old_script = Receipt(data, 'sequoia-eleven/vines-script-installation', lambda name: name.startswith('kwin/scripts/' + OLD_SCRIPT_ID + '/'))
    if old_script.manifest.exists():
        old_script.validate()
    previous_choice = setting(OLD_SCRIPT_ID) if old_script.manifest.exists() else ''
    legacy = Receipt(data, 'sequoia-eleven/vines-v7-installation', lambda name: name.startswith('kwin/effects/' + LEGACY_ID + '/'))
    if legacy.manifest.exists():
        legacy.validate()
    subprocess.run(['cmake', '-S', str(ROOT / 'desktop/effects/vines'), '-B', str(ROOT / 'build/vines-v9'), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release'], check=True)
    subprocess.run(['cmake', '--build', str(ROOT / 'build/vines-v9'), '--parallel', '4'], check=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-vines-install-') as temporary:
        payload = Path(temporary)
        shutil.copytree(ROOT / 'build/vines-v9/package', payload / 'kwin/scripts' / ID)
        receipt.install(payload, obsolete=[name for name in receipt.data["files"]
                         if "/native7/" in name])
    # Retire the old effect so there is only one settings entry and no hidden
    # animation instance. Never re-enable it during migration or removal.
    enabled('false', LEGACY_ID)
    run('qdbus-qt6', 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.unloadEffect', LEGACY_ID)
    if legacy.manifest.exists():
        backup = data / 'sequoia-eleven/vines-v7-upgrade-backup'
        if not backup.exists():
            shutil.copytree(legacy.state, backup / 'receipt')
            shutil.copytree(data / 'kwin/effects' / LEGACY_ID, backup / 'package')
        legacy.uninstall()
    if old_script.manifest.exists():
        enabled('false', OLD_SCRIPT_ID)
        script('unloadScript', OLD_SCRIPT_ID)
        backup = data / 'sequoia-eleven/vines-script-v8-upgrade-backup'
        if not backup.exists():
            shutil.copytree(old_script.state, backup / 'receipt')
            shutil.copytree(data / 'kwin/scripts' / OLD_SCRIPT_ID, backup / 'package')
        old_script.uninstall()
    # Preserve an explicit choice on updates; new installs stay unloaded.
    if not setting():
        enabled(previous_choice or 'false')
    # Unload the old QML tree before loading the new module. Preserve the
    # settings checkbox; fresh installs remain disabled.
    script('unloadScript', ID)
    script('start')
    print('Installed Jungle Vines in System Settings → Window Management → KWin Scripts.')
    print('Jungle Vines loaded:', script('isScriptLoaded', ID))


if __name__ == '__main__':
    main()
