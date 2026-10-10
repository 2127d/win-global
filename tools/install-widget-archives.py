#!/usr/bin/env python3
"""Install widget .tar.gz files from the current folder, unify notifications, restart Plasma.

Run as your desktop user: python3 install-widgets.py
The script requests sudo only for system package files. Existing tray/task-manager
instances remain in place; add their Sequoia forks yourself from Add Widgets.
Includes start-menu.tar.gz (Sequoia Start Menu) when present in the archive folder.
"""
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import xml.etree.ElementTree as ET

NOTIFICATIONS = 'org.sequoia.notifications'
ORIGINAL_NOTIFICATIONS = 'org.kde.plasma.notifications'
START_MENU = 'org.sequoia.startmenu'
TRAYS = ('org.kde.plasma.systemtray', 'org.sequoia.systemtray')
ID = re.compile(r'^[A-Za-z0-9][A-Za-z0-9_.-]+$')


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def package_id(path):
    metadata = json.loads((path / 'metadata.json').read_text())
    name = metadata.get('KPlugin', {}).get('Id', '')
    if (not ID.fullmatch(name) or name != path.name or name.startswith('org.kde.')
            or metadata.get('KPackageStructure') != 'Plasma/Applet'
            or not (path / 'contents/ui/main.qml').is_file()):
        raise ValueError(f'Not a complete independently named Plasma widget: {path}')
    return name


def unpack_archives(folder, stage):
    packages = {}
    for filename in sorted(folder.glob('*.tar.gz')):
        with tarfile.open(filename, 'r:gz') as archive:
            members = archive.getmembers()
            roots = {PurePosixPath(m.name).parts[0] for m in members if PurePosixPath(m.name).parts}
            if len(roots) != 1 or not any(m.name.rstrip('/') == next(iter(roots)) + '/metadata.json' for m in members):
                print(f'Skipping {filename.name}: not a widget archive.')
                continue
            if sum(m.size for m in members) > 128 * 1024 * 1024:
                raise ValueError(f'Widget archive is unexpectedly large: {filename}')
            for member in members:
                path = PurePosixPath(member.name)
                if path.is_absolute() or '..' in path.parts or not (member.isfile() or member.isdir()):
                    raise ValueError(f'Unsafe archive member in {filename.name}: {member.name}')
            extracted = stage / filename.name
            archive.extractall(extracted, filter='data')
        package = extracted / roots.pop()
        name = package_id(package)
        manifest_path = package / 'sequoia-package.json'
        if manifest_path.exists():
            manifest = json.loads(manifest_path.read_text())
            if manifest.get('format') != 1 or manifest['architecture'] != platform.machine():
                raise ValueError(f'{filename.name}: incompatible package format or CPU architecture.')
            qt_version = subprocess.check_output(['qtpaths6', '--qt-version'], text=True).strip()
            if qt_version != manifest['qt_version']:
                raise ValueError(f'{filename.name}: built for Qt {manifest["qt_version"]}, installed Qt is {qt_version}. Rebuild the archives.')
            for relative, expected in manifest['files'].items():
                path = package / relative
                if not path.resolve().is_relative_to(package.resolve()) or checksum(path) != expected:
                    raise ValueError(f'{filename.name}: checksum mismatch: {relative}')
            for source, expected in manifest['upstream_sha256'].items():
                if not Path(source).is_file() or checksum(Path(source)) != expected:
                    raise ValueError(f'{filename.name}: KDE backend differs from this build ({source}). '
                                     'Rebuild these archives on the target Plasma version before installing.')
        children = sorted((package / 'contents/bundled').glob('*')) if (package / 'contents/bundled').exists() else []
        for candidate in [package, *children]:
            name = package_id(candidate)
            if name in packages:
                raise ValueError(f'Duplicate widget ID: {name}')
            if (candidate / 'contents/native').exists() and not manifest_path.exists():
                raise ValueError(f'Native widget has no compatibility manifest: {name}')
            packages[name] = candidate
        print(f'Validated {filename.name}')
    if not packages:
        raise ValueError('No valid widget .tar.gz files were found in the current folder.')
    return packages


def plugin_directory():
    value = subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip()
    path = Path(value)
    if not path.is_absolute():
        raise ValueError('Qt returned an invalid plugin directory.')
    return path / 'plasma/applets'


def remove(path):
    if path.is_symlink() or path.is_file(): path.unlink()
    elif path.exists(): shutil.rmtree(path)


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    if source.is_dir():
        shutil.copytree(source, target, symlinks=True)
    else:
        shutil.copy2(source, target, follow_symlinks=False)


def install_files(packages, prefix):
    targets = []
    applets = prefix / 'usr/share/plasma/plasmoids'
    native = prefix / plugin_directory().relative_to('/')
    for name, source in packages.items():
        targets.append((source, applets / name))
        library = source / 'contents/native' / (name + '.so')
        if library.is_file(): targets.append((library, native / library.name))
    backup = prefix / 'var/backups/sequoia-widgets' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True)
    for index, (_, target) in enumerate(targets):
        if target.exists() or target.is_symlink(): copy(target, backup / str(index))
    (backup / 'manifest.json').write_text(json.dumps([str(target) for _, target in targets], indent=2) + '\n')
    changed = []
    try:
        for index, (source, target) in enumerate(targets):
            target.parent.mkdir(parents=True, exist_ok=True)
            # Build outside Plasma's recursively scanned package directory.
            with tempfile.TemporaryDirectory(prefix='.sequoia-install-', dir=target.parent.parent) as temporary:
                incoming = Path(temporary) / target.name
                copy(source, incoming)
                if incoming.is_dir():
                    remove(incoming / 'contents/bundled')
                changed.append(index)
                remove(target)
                incoming.rename(target)
    except Exception:
        for index in reversed(changed):
            target = targets[index][1]
            remove(target)
            if (backup / str(index)).exists(): copy(backup / str(index), target)
        raise
    print(f'Installed {len(packages)} independent packages. Previous system files: {backup}')


def notification_settings(text, packages):
    """Preserve all unrelated KConfig text, widget IDs, positions and launchers."""
    blocks = re.split(r'(?m)^(\[[^\n]+\])\s*\n', text)
    sections = {blocks[i]: blocks[i + 1] for i in range(1, len(blocks), 2)}
    order = list(sections)
    parents = [header for header, body in sections.items()
               if re.search(r'^plugin=(?:org\.kde\.plasma\.systemtray|org\.sequoia\.systemtray)\s*$', body, re.M)]
    defaults = {'extraItems': [], 'knownItems': []}
    tray = packages.get('org.sequoia.systemtray')
    aliases = {name: json.loads((path / 'metadata.json').read_text()).get('X-Sequoia-NativePlugin', name)
               for name, path in packages.items()}
    if tray:
        schema = ET.parse(tray / 'contents/config/main.xml')
        for key in defaults:
            node = schema.find('.//{*}entry[@name="' + key + '"]/{*}default')
            defaults[key] = (node.text or '').split(',')

    def get(body, key, fallback):
        match = re.search(r'^' + key + r'=(.*)$', body, re.M)
        return match.group(1).split(',') if match else list(fallback)

    def set_value(body, key, values):
        value = key + '=' + ','.join(dict.fromkeys(v for v in values if v))
        pattern = r'^' + key + r'=.*$'
        if re.search(pattern, body, re.M): return re.sub(pattern, lambda _: value, body, flags=re.M)
        return body.rstrip('\n') + '\n' + value + '\n'

    for parent in parents:
        fork = 'plugin=org.sequoia.systemtray' in sections[parent]
        header = parent + '[Configuration][General]'
        if parent + '[General]' in sections or header not in sections:
            header = parent + '[General]'
        body = sections.get(header, '')
        enabled_default = defaults['extraItems'] if fork else [aliases.get(v, v) for v in defaults['extraItems']]
        enabled = get(body, 'extraItems', enabled_default)
        known = get(body, 'knownItems', [])
        use_notifications = ORIGINAL_NOTIFICATIONS in enabled or NOTIFICATIONS in enabled or ORIGINAL_NOTIFICATIONS not in known
        enabled = [v for v in enabled if v not in (ORIGINAL_NOTIFICATIONS, NOTIFICATIONS)]
        if use_notifications: enabled.append(NOTIFICATIONS)
        body = set_value(body, 'extraItems', enabled)
        body = set_value(body, 'knownItems', known + [ORIGINAL_NOTIFICATIONS, NOTIFICATIONS])
        for key in ('hiddenItems', 'shownItems'):
            if re.search(r'^' + key + '=', body, re.M):
                body = set_value(body, key, [NOTIFICATIONS if v == ORIGINAL_NOTIFICATIONS else v for v in get(body, key, [])])
        if header not in sections: order.append(header)
        sections[header] = body
    # Existing notification instances retain their IDs and configuration.
    for header in sections:
        sections[header] = re.sub(r'^plugin=org\.kde\.plasma\.notifications\s*$',
                                  'plugin=' + NOTIFICATIONS, sections[header], flags=re.M)
    return blocks[0] + ''.join(header + '\n' + sections[header]
                               + ('' if sections[header].endswith('\n') else '\n') for header in order)


def configure_notifications(packages, config):
    if NOTIFICATIONS not in packages: return
    panel = config / 'plasma-org.kde.plasma.desktop-appletsrc'
    if not panel.exists():
        print('No panel configuration yet; add Sequoia System Tray to use unified notifications.')
        return
    old = panel.read_text()
    updated = notification_settings(old, packages)
    if updated != old:
        backup = config / ('sequoia-widgets-backup-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        backup.mkdir()
        shutil.copy2(panel, backup / panel.name)
        panel.write_text(updated)
        print(f'Unified notification style selected. Panel settings backup: {backup}')


def fork_settings(text, packages):
    """Migrate panel instances in place, retaining their IDs and child settings."""
    aliases = {json.loads((path / 'metadata.json').read_text()).get('X-Sequoia-NativePlugin'): name
               for name, path in packages.items()}
    aliases.pop(None, None)
    if 'org.sequoia.icontasks.animated' in packages:
        aliases['org.kde.plasma.icontasks'] = 'org.sequoia.icontasks.animated'
    blocks = re.split(r'(?m)^(\[[^\n]+\])[ \t]*\n', text)
    trays = [blocks[i] for i in range(1, len(blocks), 2)
             if re.search(r'^plugin=(?:org\.kde\.plasma\.systemtray|org\.sequoia\.systemtray)$', blocks[i + 1], re.M)]
    panel_ids = {'org.kde.plasma.systemtray', 'org.kde.plasma.icontasks', 'org.kde.plasma.trash'}
    for i in range(1, len(blocks), 2):
        header, body = blocks[i:i + 2]
        in_tray = any(header == tray or header.startswith(tray + '[') for tray in trays)
        for old, new in aliases.items():
            if in_tray or old in panel_ids:
                body = re.sub(r'^plugin=' + re.escape(old) + r'$', 'plugin=' + new, body, flags=re.M)
        if in_tray and header.endswith('[General]') and any(
                header == tray + suffix for tray in trays for suffix in ('[General]', '[Configuration][General]')):
            for key in ('extraItems', 'hiddenItems', 'shownItems', 'knownItems'):
                match = re.search(r'^' + key + r'=(.*)$', body, re.M)
                if match:
                    old = match.group(1).split(',')
                    values = [aliases.get(value, value) for value in old]
                    if key == 'knownItems':
                        values = old + values + list(aliases) + list(aliases.values())
                    value = ','.join(dict.fromkeys(v for v in values if v))
                    body = body[:match.start(1)] + value + body[match.end(1):]
        blocks[i + 1] = body
    return blocks[0] + ''.join(blocks[i] + '\n' + blocks[i + 1] for i in range(1, len(blocks), 2))


def configure_forks(packages, config):
    panel = config / 'plasma-org.kde.plasma.desktop-appletsrc'
    if not panel.exists():
        return
    old = panel.read_text()
    updated = fork_settings(old, packages)
    if updated != old:
        backup = config / ('sequoia-widgets-backup-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        backup.mkdir()
        shutil.copy2(panel, backup / panel.name)
        panel.write_text(updated)
        print(f'Custom tray, trash and task manager selected. Panel backup: {backup}')


def retire_local_copies(packages, data, config, activate_forks=False):
    """Retire only package IDs being installed and our recognized legacy hook."""
    paths = [data / 'plasma/plasmoids' / name for name in packages]
    legacy = config / 'systemd/user/plasma-plasmashell.service.d/50-sequoia-control-center.conf'
    if (activate_forks and 'org.sequoia.systemtray' in packages and 'org.sequoia.icontasks.animated' in packages
            and legacy.is_file() and '# Sequoia Eleven Control Center' in legacy.read_text()):
        paths.append(legacy)
    paths = [p for p in paths if p.exists() or p.is_symlink()]
    if not paths: return
    backup = data / 'sequoia-widgets-backups' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True)
    for index, path in enumerate(paths):
        copy(path, backup / str(index))
    (backup / 'manifest.json').write_text(json.dumps([str(p) for p in paths], indent=2) + '\n')
    for path in paths: remove(path)
    print(f'Previous local package copies and legacy overlay hook saved to {backup}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--folder', type=Path, default=Path.cwd(), help='Archive folder (default: current working directory)')
    parser.add_argument('--check', action='store_true', help='Validate archives and compatibility only')
    parser.add_argument('--activate-forks', action='store_true',
                        help='Switch existing tray, trash and task-manager instances to their custom forks, keeping settings')
    parser.add_argument('--prefix', type=Path, help='Stage system files under this directory, without sudo or restarting Plasma')
    parser.add_argument('--config-dir', type=Path, help='Optional configuration directory for staged testing')
    parser.add_argument('--system-files', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    folder = args.folder.resolve()
    with tempfile.TemporaryDirectory(prefix='sequoia-archive-install-') as temporary:
        packages = unpack_archives(folder, Path(temporary))
        if args.check:
            if START_MENU in packages: print('Sequoia Start Menu is ready to install.')
            return
        if args.prefix:
            install_files(packages, args.prefix.resolve())
            if args.config_dir:
                if args.activate_forks: configure_forks(packages, args.config_dir.resolve())
                else: configure_notifications(packages, args.config_dir.resolve())
            return
        if args.system_files:
            if os.geteuid() != 0: raise ValueError('System file installation requires sudo.')
            install_files(packages, Path('/'))
            return
        if os.geteuid() == 0:
            raise ValueError('Run this script as your desktop user, without sudo. It elevates only the file installation.')
        subprocess.run(['sudo', sys.executable, str(Path(__file__).resolve()), '--folder', str(folder), '--system-files'], check=True)
        config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
        data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
        # Stop before editing, so Plasma cannot write its old settings over the
        # notification selection. Start again even if settings editing fails.
        subprocess.run(['systemctl', '--user', 'stop', 'plasma-plasmashell.service'], check=True)
        try:
            if args.activate_forks: configure_forks(packages, config)
            else: configure_notifications(packages, config)
            retire_local_copies(packages, data, config, args.activate_forks)
        finally:
            try:
                subprocess.run(['systemctl', '--user', 'daemon-reload'], check=True)
            finally:
                subprocess.run(['systemctl', '--user', 'start', 'plasma-plasmashell.service'], check=True)
        print('Installed. Custom panel widgets selected; settings retained.' if args.activate_forks else
              'Installed. Existing System Tray and task-manager instances and settings are retained. Add the Sequoia forks from Add Widgets.')
        if START_MENU in packages:
            print('Add Widgets → Sequoia Start Menu. Use its gear button to configure layout, pinned apps, and animations.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, tarfile.TarError) as error:
        raise SystemExit(f'Installation failed: {error}')
