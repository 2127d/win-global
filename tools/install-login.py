#!/usr/bin/env python3
"""Install matching Plasma Login colors, fonts, wallpaper, and light/dark sync."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import pwd
import shutil
import subprocess
import tempfile

from login_theme import apply_appearance, desktop_mode, read_config, save_config, set_values

ROOT = Path(__file__).resolve().parents[1]
SERVICE = 'sequoia-eleven-login-sync'


def stage(prefix, source_config, mode, greeter_home):
    def target(path):
        return prefix / str(path).lstrip('/')

    data = target('/usr/local/share')
    for mode_name in ('light', 'dark'):
        for category, name in [('desktoptheme', f'sequoia-eleven-{mode_name}'),
                               ('look-and-feel', f'org.sequoia.eleven.{mode_name}')]:
            shutil.copytree(ROOT/'themes/plasma'/category/name, data/'plasma'/category/name)
        palette = f'SequoiaEleven{mode_name.title()}.colors'
        (data/'color-schemes').mkdir(exist_ok=True)
        shutil.copy2(ROOT/'themes/color-schemes'/palette, data/'color-schemes'/palette)
    shutil.copytree(ROOT/'themes/wallpapers/SequoiaEleven', data/'wallpapers/SequoiaEleven')
    for family in ('AdwaitaSans', 'Nunito'):
        shutil.copytree(ROOT/'themes/fonts'/family, data/'fonts/sequoia-eleven-login'/family)

    config = target('/etc/plasmalogin.conf')
    # Preserve authentication, session, and other unrelated settings.
    current = read_config(Path('/etc/plasmalogin.conf'))
    set_values(current, 'Greeter', {'WallpaperPluginId': 'org.kde.image', 'ShowClock': 'true'})
    set_values(current, 'Greeter][Wallpaper][org.kde.image][General', {
        'Image': 'file:///usr/local/share/wallpapers/SequoiaEleven',
        'PreviewImage': '', 'FillMode': '2', 'DynamicMode': '0'})
    save_config(current, config)
    config.parent.mkdir(parents=True, exist_ok=True)
    target('/etc/sequoia-eleven-login.json').write_text(json.dumps({
        'source_config': str(source_config)}, indent=2) + '\n')

    helper = target('/usr/local/libexec/sequoia-eleven-login-sync.py')
    helper.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT/'tools/login_theme.py', helper)
    units = target('/etc/systemd/system')
    units.mkdir(parents=True, exist_ok=True)
    (units/f'{SERVICE}.service').write_text('''[Unit]
Description=Match Sequoia Eleven login appearance to the desktop

[Service]
Type=oneshot
ExecStart=/usr/bin/python3 -I /usr/local/libexec/sequoia-eleven-login-sync.py
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ReadWritePaths=/var/lib/plasmalogin
'''.replace('/var/lib/plasmalogin', str(greeter_home)))
    (units/f'{SERVICE}.path').write_text(f'''[Unit]
Description=Watch desktop light/dark appearance for the login screen

[Path]
PathChanged={source_config}/kdeglobals
PathChanged={source_config}/kdedefaults/kdeglobals
Unit={SERVICE}.service

[Install]
WantedBy=multi-user.target
''')
    # Startup sync also covers changes made while the path watcher was inactive.
    (units/f'{SERVICE}.service').write_text((units/f'{SERVICE}.service').read_text() +
                                         '\n[Install]\nWantedBy=multi-user.target\n')
    greeter_config = target(greeter_home/'.config')
    for name in ('kdeglobals', 'plasmarc'):
        existing = greeter_home/'.config'/name
        if existing.is_file() and os.geteuid() == 0:
            greeter_config.mkdir(parents=True, exist_ok=True)
            shutil.copy2(existing, greeter_config/name)
    apply_appearance(greeter_config, data, mode)


def install(staged, greeter_home):
    backup = Path('/var/backups/sequoia-eleven-login')/datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True, mode=0o700)
    manifest = []
    # Every overwritten file has a copy, including previous installations.
    for source in sorted(staged.rglob('*')):
        if not source.is_file():
            continue
        destination = Path('/')/source.relative_to(staged)
        if destination.is_symlink() or any(p.is_symlink() for p in destination.parents):
            raise RuntimeError(f'Refusing to install through a symlink: {destination}')
        entry = {'path': str(destination), 'exists': destination.exists(), 'copy': str(len(manifest))}
        if destination.exists():
            info = destination.stat()
            entry.update(uid=info.st_uid, gid=info.st_gid, mode=info.st_mode & 0o777)
            shutil.copy2(destination, backup/entry['copy'])
        manifest.append(entry)
    states = {}
    for suffix in ('service', 'path'):
        unit = f'{SERVICE}.{suffix}'
        states[unit] = subprocess.run(['systemctl', 'is-enabled', unit], capture_output=True, text=True).stdout.strip()
    (backup/'manifest.json').write_text(json.dumps({'files': manifest, 'units': states}, indent=2)+'\n')
    print(f'Backup: {backup}', flush=True)
    account = pwd.getpwnam('plasmalogin')
    for entry in manifest:
        destination = Path(entry['path'])
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(staged/destination.relative_to('/'), destination)
        os.chmod(destination, 0o644)
        if destination.is_relative_to(greeter_home):
            os.chown(destination, account.pw_uid, account.pw_gid)
            os.chown(destination.parent, account.pw_uid, account.pw_gid)
    subprocess.run(['restorecon', '-R', str(greeter_home/'.config'), '/usr/local/share/fonts/sequoia-eleven-login',
                    '/etc/plasmalogin.conf'], check=True)
    subprocess.run(['fc-cache', '-f', '/usr/local/share/fonts/sequoia-eleven-login'], check=True)
    subprocess.run(['systemctl', 'daemon-reload'], check=True)
    subprocess.run(['systemctl', 'enable', '--now', f'{SERVICE}.path', f'{SERVICE}.service'], check=True)
    print('Installed. The matching appearance will be used at the next login screen.')
    print(f'Restore: sudo python3 {ROOT}/tools/install-login.py --restore {backup}')


def restore(backup):
    manifest = json.loads((backup/'manifest.json').read_text())
    subprocess.run(['systemctl', 'disable', '--now', f'{SERVICE}.path', f'{SERVICE}.service'], check=True)
    for entry in reversed(manifest['files']):
        destination = Path(entry['path'])
        if entry['exists']:
            shutil.copyfile(backup/entry['copy'], destination)
            os.chmod(destination, entry['mode'])
            os.chown(destination, entry['uid'], entry['gid'])
        else:
            destination.unlink(missing_ok=True)
    subprocess.run(['systemctl', 'daemon-reload'], check=True)
    for unit, state in manifest['units'].items():
        if state == 'enabled':
            subprocess.run(['systemctl', 'enable', '--now', unit], check=True)
    print('Restored the previous login appearance. Changes appear at the next login screen.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--prefix', type=Path, help='Stage the system files without installing them.')
    action.add_argument('--install', action='store_true')
    action.add_argument('--restore', type=Path)
    parser.add_argument('--user', help='Desktop account whose Sequoia Eleven light/dark mode to follow.')
    parser.add_argument('--config-dir', type=Path,
                        help='Desktop account configuration directory when XDG_CONFIG_HOME is customized (also pass it through sudo).')
    parser.add_argument('--mode', choices=('light', 'dark'), help='Initial mode; default: current desktop mode.')
    args = parser.parse_args()
    if not args.prefix and os.geteuid() != 0:
        parser.error('System installation/restoration requires administrator authentication (sudo or pkexec).')
    if args.restore:
        restore(args.restore.resolve())
        return
    user = args.user or os.environ.get('SUDO_USER') or pwd.getpwuid(os.getuid()).pw_name
    account = pwd.getpwnam(user)
    if account.pw_uid == 0:
        parser.error('Use --user to select the desktop account.')
    # sudo normally removes XDG_CONFIG_HOME. Never accidentally follow root's
    # or a different caller's settings when --user selects another account.
    current_config = os.environ.get('XDG_CONFIG_HOME') if account.pw_uid == os.getuid() else None
    source_config = args.config_dir or Path(current_config or Path(account.pw_dir)/'.config')
    if not source_config.is_absolute():
        parser.error('The desktop configuration directory must be an absolute path.')
    if any(character in str(source_config) for character in '\n\r%'):
        parser.error('Unsupported characters in the desktop configuration path.')
    greeter_home = Path(pwd.getpwnam('plasmalogin').pw_dir)
    mode = args.mode or desktop_mode(source_config)
    if mode is None:
        parser.error('Select Sequoia Eleven first or specify --mode light/dark.')
    if args.prefix:
        args.prefix.mkdir(parents=True, exist_ok=True)
        stage(args.prefix.resolve(), source_config, mode, greeter_home)
        print(f'Staged Sequoia Eleven {mode.title()} login appearance in {args.prefix}')
    else:
        if not Path('/usr/lib64/qt6/plugins/styles/sequoiaeleven6.so').exists():
            parser.error('Install the SequoiaEleven application style first (see README.md).')
        with tempfile.TemporaryDirectory(prefix='sequoia-login-install-') as temporary:
            stage(Path(temporary), source_config, mode, greeter_home)
            install(Path(temporary), greeter_home)


if __name__ == '__main__':
    main()
