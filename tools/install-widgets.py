#!/usr/bin/env python3
"""Build, install or restore the Sequoia widget collection for the current user."""
import argparse
from datetime import datetime
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ('org.sequoia.downloads', 'org.sequoia.dockseparator', 'org.sequoia.startmenu')


def install_package(source, package_root):
    # KPackage checks the package structure and notifies KDE
    # about installation. A successful directory copy alone proves neither.
    subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet',
                    '--packageroot', str(package_root), '--install', str(source)], check=True)


def check_packages(data):
    package_root = data / 'plasma/plasmoids'
    for name in PACKAGES:
        package = package_root / name
        metadata = json.loads((package / 'metadata.json').read_text())
        if metadata.get('KPlugin', {}).get('Id') != name:
            raise RuntimeError(f'Widget ID does not match its installed folder: {package}')
        if not (package / 'contents/ui/main.qml').is_file():
            raise RuntimeError(f'Missing widget entry point: {package}/contents/ui/main.qml')
        subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet',
                        '--show', str(package)], check=True)


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    if source.is_dir():
        shutil.copytree(source, target)
    else:
        shutil.copy2(source, target)


def remove(path):
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.exists():
        shutil.rmtree(path)


def destinations(data, config):
    return [data / 'sequoia-eleven/control-center',
            *(data / 'plasma/plasmoids' / name for name in PACKAGES),
            config / 'systemd/user/plasma-plasmashell.service.d/50-sequoia-control-center.conf']


def restore(backup, targets):
    manifest = json.loads((backup / 'backup.json').read_text())
    if manifest['targets'] != [str(p) for p in targets]:
        raise RuntimeError('Backup belongs to different installation paths.')
    for index, target in enumerate(targets):
        remove(target)
        saved = backup / str(index)
        if saved.exists():
            copy(saved, target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--stage', type=Path, help='Build a relocatable data tree without changing the desktop')
    mode.add_argument('--install', action='store_true', help='Install per-user overrides and packages, with backup')
    mode.add_argument('--restore', type=Path, help='Restore a backup created by this script')
    mode.add_argument('--check', action='store_true', help='Check installed widget IDs, paths and entry points without changing the desktop')
    parser.add_argument('--restart', action='store_true', help='Restart Plasma after install or restore')
    args = parser.parse_args()
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share').resolve()
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config').resolve()
    targets = destinations(data, config)
    if args.check:
        check_packages(data)
        return
    if args.restore:
        restore(args.restore.resolve(), targets)
    else:
        # All integration checks and compilation happen before installed files change.
        spec = importlib.util.spec_from_file_location('widget_builder', ROOT / 'tools/install-control-center.py')
        builder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(builder)
        with tempfile.TemporaryDirectory(prefix='sequoia-widgets-build-') as temporary:
            stage = Path(temporary)
            overlay = stage / 'sequoia-eleven/control-center'
            builder.build(overlay)
            shutil.copy2(builder.SOURCE / 'launch.py', overlay / 'launch.py')
            for name in PACKAGES:
                source = ROOT / 'plasma/plasmoids' / name
                install_package(source, stage / 'plasma/plasmoids')
            check_packages(stage)
            if args.stage:
                if args.stage.exists():
                    raise RuntimeError('Stage destination must not already exist.')
                copy(stage, args.stage.resolve())
                print(f'Built and checked widgets: {args.stage.resolve()}')
                return
            if targets[0].exists() and not (targets[0] / builder.MARKER).exists():
                raise RuntimeError(f'Unmanaged installation: {targets[0]}')
            if targets[-1].exists() and '# Sequoia Eleven Control Center' not in targets[-1].read_text():
                raise RuntimeError(f'Unmanaged service override: {targets[-1]}')
            backup = data / 'sequoia-eleven/widget-backups' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
            backup.mkdir(parents=True)
            for index, target in enumerate(targets):
                if target.exists():
                    copy(target, backup / str(index))
            (backup / 'backup.json').write_text(json.dumps({'targets': [str(p) for p in targets]}, indent=2) + '\n')
            try:
                target = targets[0]
                # Stage on the destination filesystem; never overwrite a mapped .so.
                with tempfile.TemporaryDirectory(prefix='.widget-install-', dir=target.parent) as incoming:
                    new = Path(incoming) / 'package'
                    copy(overlay, new)
                    remove(target)
                    new.rename(target)
                for name, target in zip(PACKAGES, targets[1:-1]):
                    # Backups above allow reinstalling even the same version or
                    # repairing an incomplete package, with rollback on failure.
                    remove(target)
                    install_package(stage / 'plasma/plasmoids' / name, target.parent)
                check_packages(data)
                launcher = str(targets[0] / 'launch.py').replace('%', '%%').replace('"', '\\"')
                targets[-1].parent.mkdir(parents=True, exist_ok=True)
                targets[-1].write_text('# Sequoia Eleven Control Center\n[Service]\nExecStart=\nExecStart=/usr/bin/python3 "' + launcher + '"\n')
            except Exception:
                restore(backup, targets)
                raise
            print(f'Installed all five widgets. Backup: {backup}')
    subprocess.run(['systemctl', '--user', 'daemon-reload'], check=True)
    if args.restart:
        subprocess.run(['systemctl', '--user', 'restart', 'plasma-plasmashell.service'], check=True)
    else:
        print('Before adding widgets, restart Plasma with systemctl --user restart plasma-plasmashell.service, or log out and back in.')
    print('Existing System Tray and Icons-only Task Manager instances use the overlay. After restarting Plasma, add Downloads Stack, Dock Separator, and Sequoia Start Menu from Add Widgets if needed. Remove and re-add any instances still showing an earlier loading error.')


if __name__ == '__main__':
    main()
