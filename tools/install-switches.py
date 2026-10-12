#!/usr/bin/env python3
"""Refresh switch styling in an existing tracked Sequoia installation."""
import argparse
import copy
import hashlib
import importlib.util
import os
from pathlib import Path
import tempfile

from quick_islands import QML, replace
from menu_toggles import desktop_menu
from theme_files import Receipt, copy_file, exists, fingerprint

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('theme_installer', ROOT / 'tools/install-theme.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)
STATE = installer.USER_STATE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, help='Alternate data directory, or system root with --native.')
    parser.add_argument('--native', action='store_true', help='Refresh the staged native menu style (requires root for /).')
    args = parser.parse_args()
    if args.native:
        prefix = (args.prefix or Path('/')).absolute()
        name = 'usr/lib64/qt6/plugins/styles/sequoiaeleven6.so'
        targets = {name: ROOT / '.staging/desktop' / name}
        if not (prefix / installer.SYSTEM_STATE / 'manifest.json').is_file():
            raise RuntimeError('Run ./install.sh first; no tracked native installation exists.')
        receipt = Receipt(prefix, installer.SYSTEM_STATE, installer.system_allowed)
    else:
        prefix = (args.prefix or Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')).absolute()
        targets = {}
        for mode in ('light', 'dark'):
            theme = f'plasma/desktoptheme/sequoia-eleven-{mode}'
            targets[f'{theme}/widgets/switch.svg'] = ROOT / theme / 'widgets/switch.svg'
            targets[f'{theme}/colors'] = None
        indicator = 'sequoia-eleven/quick-islands/qml/org/kde/desktop/private/SwitchIndicator.qml'
        targets[indicator] = ROOT / 'application-style/quick/SwitchIndicator.qml'
        menu = 'sequoia-eleven/quick-islands/qml/org/kde/desktop/MenuItem.qml'
        targets[menu] = QML / 'org/kde/desktop/MenuItem.qml'
        if not (prefix / STATE / 'manifest.json').is_file() or not (prefix / indicator).is_file():
            raise RuntimeError('Run ./install.sh first; this command refreshes an existing tracked installation.')
        receipt = Receipt(prefix, STATE, installer.user_allowed)
    original_data = copy.deepcopy(receipt.data)
    # Validate only this change's files. Other independently developed project
    # components may already have been refreshed with their own installers.
    for name, source in targets.items():
        target = receipt.target(name)
        entry = receipt.data['files'].get(name)
        if entry and fingerprint(target) not in (None, entry['installed']):
            raise RuntimeError(f'Preserve local edits before refreshing switches: {target}')
        if source and not source.is_file():
            raise RuntimeError(f'Missing switch source: {source}')
    with tempfile.TemporaryDirectory(prefix='switch-refresh-') as temporary:
        if not args.native:
            prepared_menu = Path(temporary) / 'MenuItem.qml'
            prepared_menu.write_text(desktop_menu(targets[menu].read_text(), replace))
            targets[menu] = prepared_menu
        previous = {}
        for index, name in enumerate(targets):
            target = receipt.target(name)
            backup = Path(temporary) / str(index)
            previous[name] = backup if exists(target) else None
            if exists(target):
                copy_file(target, backup)
        try:
            for name, source in targets.items():
                target = receipt.target(name)
                if name not in receipt.data['files']:
                    original = None
                    if exists(target):
                        original = hashlib.sha256(name.encode()).hexdigest()
                        backup = receipt.state / 'originals' / original
                        receipt.check_parents(backup)
                        backup.parent.mkdir(parents=True, exist_ok=True)
                        copy_file(target, backup)
                    receipt.data['files'][name] = {'original': original, 'installed': None}
                receipt.data['files'][name]['installed'] = fingerprint(source) if source else None
                receipt.mkdirs(target.parent)
            receipt.save()
            for name, source in targets.items():
                target = receipt.target(name)
                if source:
                    copy_file(source, target)
                else:
                    target.unlink(missing_ok=True)
        except BaseException:
            for name, backup in previous.items():
                target = receipt.target(name)
                target.unlink(missing_ok=True)
                if backup:
                    copy_file(backup, target)
            receipt.data = original_data
            receipt.save()
            raise
    print('Installed accent-aware native menu style.' if args.native else 'Installed accent-aware switches and Quick menu indicators.')
    print('Installation ownership and original backups preserved.')
    print('Reopen styled applications. Log out and back in to reload cached Plasma SVG artwork.')


if __name__ == '__main__':
    main()
