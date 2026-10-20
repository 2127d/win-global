#!/usr/bin/env python3
"""Repair the active Plasma Style's progress artwork for stock task managers."""
import argparse
from datetime import datetime
import gzip
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from task_progress import without_progress_endcaps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--theme', help='Plasma Style ID (defaults to the active style)')
    parser.add_argument('--restart-plasma', action='store_true', help='Briefly restart the panel to load the fix')
    args = parser.parse_args()
    theme = args.theme or subprocess.check_output(
        ['kreadconfig6', '--file', 'plasmarc', '--group', 'Theme', '--key', 'name', '--default', 'default'],
        text=True).strip()
    if not theme or theme in ('.', '..') or '/' in theme or '\\' in theme:
        parser.error('Invalid Plasma Style ID')
    data_home = Path(os.environ.get('XDG_DATA_HOME', str(Path.home() / '.local/share')))
    roots = [data_home, *(Path(p) for p in os.environ.get('XDG_DATA_DIRS', '/usr/local/share:/usr/share').split(':') if p)]

    def locate(theme_id):
        return next((root / 'plasma/desktoptheme' / theme_id for root in roots
                     if (root / 'plasma/desktoptheme' / theme_id).is_dir()), None)

    source = locate(theme)
    if source is None:
        parser.error(f'Cannot find Plasma Style {theme!r}')
    target = data_home / 'plasma/desktoptheme' / theme
    if target.is_symlink():
        parser.error(f'Refusing to replace symlink: {target}')
    with tempfile.TemporaryDirectory(prefix='task-progress-fix-') as temporary:
        stage = Path(temporary) / theme
        shutil.copytree(source, stage)
        candidates = [*stage.rglob('tasks.svg'), *stage.rglob('tasks.svgz')]
        if not candidates:
            parser.error('This style inherits its task artwork. Run again with --theme default to repair the fallback style.')
        changed = []
        for path in candidates:
            compressed = path.suffix == '.svgz'
            original = gzip.decompress(path.read_bytes()).decode() if compressed else path.read_text()
            updated = without_progress_endcaps(original)
            if updated != original:
                path.write_bytes(gzip.compress(updated.encode(), mtime=0) if compressed else updated.encode())
                changed.append(path.relative_to(stage))
        if changed:
            metadata = stage / 'metadata.json'
            if metadata.exists():
                info = json.loads(metadata.read_text())
                plugin = info.setdefault('KPlugin', {})
                plugin['Version'] = str(plugin.get('Version', '1')) + '.progress1'
                metadata.write_text(json.dumps(info, indent=2) + '\n')
            state = Path(os.environ.get('XDG_STATE_HOME', str(Path.home() / '.local/state')))
            backup = state / 'sequoia/backups' / ('task-progress-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
            backup.mkdir(parents=True)
            if target.exists():
                shutil.copytree(target, backup / theme)
                (backup / 'restore.txt').write_text(f'Restore this backed-up theme folder to {target}\n')
            else:
                (backup / 'restore.txt').write_text(f'Remove {target} to restore the unchanged system style.\n')
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(stage, target, dirs_exist_ok=True)
            print(f'Fixed {len(changed)} progress artwork file(s) in {target}')
            print(f'Backup/restore instructions: {backup}')
        else:
            print(f'{theme}: no overlapping progress end caps found; no files changed.')
    if args.restart_plasma:
        subprocess.run(['systemctl', '--user', 'restart', 'plasma-plasmashell.service'], check=True)
        print('Plasma restarted.')
    elif changed:
        print('Log out and back in, or restart Plasma, to load the change.')


if __name__ == '__main__':
    main()
