#!/usr/bin/env python3
"""Export five independent widget archives and one quick installer."""
import argparse
from datetime import datetime
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FILES = {'downloads.tar.gz', 'separator.tar.gz', 'icons-only-task-manager-animated.tar.gz',
         'system-tray.tar.gz', 'start-menu.tar.gz', 'install-widgets.py'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, default=Path.home() / 'sandbox/widgets')
    args = parser.parse_args()
    destination = args.destination.resolve()
    if destination == ROOT or ROOT.is_relative_to(destination) or any(
            destination.is_relative_to(ROOT / folder) for folder in ('themes', 'widgets', 'desktop', 'apps', 'tools', 'docs')):
        raise RuntimeError('Choose a separate export folder.')
    if destination.exists() and any(destination.iterdir()):
        previous_export = (destination / '.sequoia-widgets').exists() or {p.name for p in destination.iterdir()} <= FILES
        if not previous_export:
            raise RuntimeError('Export folder contains unrelated files.')
    with tempfile.TemporaryDirectory(prefix='sequoia-widget-export-') as temporary:
        stage = Path(temporary) / 'widgets'
        subprocess.run(['python3', str(ROOT / 'tools/build-widget-forks.py'), '--destination', str(stage)], check=True)
        subprocess.run(['python3', str(stage / 'install-widgets.py'), '--folder', str(stage), '--check'], check=True)
        if destination.exists() and any(destination.iterdir()):
            backup = destination.with_name(destination.name + '-backup-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
            destination.rename(backup)
            print(f'Previous export saved to {backup}')
        shutil.copytree(stage, destination, dirs_exist_ok=True)
    print(f'Exported five .tar.gz widgets and install-widgets.py to {destination}')


if __name__ == '__main__':
    main()
