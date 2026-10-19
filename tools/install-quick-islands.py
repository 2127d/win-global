#!/usr/bin/env python3
"""Install reversible launchers and floating QML surfaces for KDE applications."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from quick_islands import build, ROOT

APPS = {
    'systemmonitor': 'org.kde.plasma-systemmonitor.desktop',
    'systemsettings': 'systemsettings.desktop',
    'kinfocenter': 'org.kde.kinfocenter.desktop',
    'discover': 'org.kde.discover.desktop',
    'merkuro': 'org.kde.merkuro.calendar.desktop',
    'merkuro-legacy': 'org.kde.merkuro.desktop',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None


def desktop_quote(path):
    # Desktop Entry Exec quoting (not shell quoting).
    return '"' + str(path).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--apps', nargs='+', choices=APPS, default=list(APPS))
    parser.add_argument('--remove', action='store_true')
    parser.add_argument('--launcher-prefix', type=Path,
                        help='When staging, write launchers for this final data directory.')
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    target = prefix / 'sequoia-eleven/quick-islands'
    marker = target / 'installation.json'
    if target.exists() and not marker.exists():
        raise RuntimeError(f'Refusing to overwrite unmanaged installation: {target}')
    installed = json.loads(marker.read_text()) if marker.exists() else {'launchers': {}}
    for name, entry in installed['launchers'].items():
        path = prefix / 'applications' / name
        if digest(path) not in (None, entry['installed_sha256']):
            raise RuntimeError(f'Launcher was edited after installation; preserve those changes before continuing: {path}')
    if args.remove:
        for name, entry in installed['launchers'].items():
            path = prefix / 'applications' / name
            if entry['original'] is None:
                path.unlink(missing_ok=True)
            else:
                path.write_bytes(base64.b64decode(entry['original']))
        if target.exists():
            shutil.rmtree(target)
        print('Removed Quick islands and restored the previous application launchers.')
    else:
        # Build/validate all transformations before touching desktop launchers.
        try:
            build(target / 'qml')
        except Exception:
            if not marker.exists() and target.exists():
                shutil.rmtree(target)
            raise
        launcher = target / 'sequoia-quick'
        shutil.copy2(ROOT / 'themes/application-style/quick/sequoia-quick', launcher)
        launcher.chmod(0o755)
        for app in args.apps:
            name = APPS[app]
            source = Path('/usr/share/applications') / name
            if not source.exists():
                print(f'Skipping {app}: not installed')
                continue
            destination = prefix / 'applications' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            entry = installed['launchers'].get(name)
            if entry is None:
                entry = {'original': base64.b64encode(destination.read_bytes()).decode() if destination.exists() else None}
            original = base64.b64decode(entry['original']).decode() if entry['original'] else source.read_text()
            lines = []
            for line in original.splitlines():
                if line.startswith('Exec='):
                    installed_launcher = (args.launcher_prefix.resolve() / 'sequoia-eleven/quick-islands/sequoia-quick'
                                          if args.launcher_prefix else launcher)
                    line = 'Exec=' + desktop_quote(installed_launcher) + ' ' + line[5:]
                if line == 'DBusActivatable=true':
                    line = 'DBusActivatable=false'
                lines.append(line)
            destination.write_text('\n'.join(lines) + '\n')
            entry['installed_sha256'] = digest(destination)
            installed['launchers'][name] = entry
        marker.write_text(json.dumps(installed, indent=2) + '\n')
        print('Installed floating QML surfaces for:', ', '.join(installed['launchers']))
        print('Close and reopen these applications from the application menu.')
        print(f'Other Qt Quick apps: {launcher} APPLICATION')
    # Refresh only for the active user's data directory, not staging/test prefixes.
    if prefix == Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').resolve():
        cache = shutil.which('kbuildsycoca6')
        if cache:
            subprocess.run([cache, '--noincremental'], check=True)


if __name__ == '__main__':
    main()
