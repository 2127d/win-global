#!/usr/bin/env python3
"""Refresh only the widget picker in an existing managed Sequoia shell."""
import argparse
from datetime import datetime, timezone
import importlib.util
import os
from pathlib import Path
import shutil
import tempfile

import shell_overrides as shell
import widget_picker
from theme_files import Receipt

SPEC = importlib.util.spec_from_file_location('theme_installer', Path(__file__).with_name('install-theme.py'))
installer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(installer)
NAMES = {shell.SHELL + '/' + name for name in widget_picker.FILES}


class PickerReceipt(Receipt):
    def validate(self, incoming=None, force=False, scope=None):
        # Other components may have intentional local edits. Leave their files
        # AND recorded hashes untouched; validate every file we actually change.
        if incoming is not None and not set(incoming) <= NAMES:
            raise RuntimeError('Unexpected file in widget-picker payload.')
        super().validate(incoming, force, scope=lambda name: name in NAMES)


def install(prefix, check=False):
    receipt = PickerReceipt(prefix, installer.USER_STATE, installer.user_allowed)
    if 'plasma-style' not in shell.installed(receipt.data['files']):
        raise RuntimeError('Install the Sequoia Plasma style first with tools/install-theme.py.')
    for name in widget_picker.FILES[:-1]:
        if shell.SHELL + '/' + name not in receipt.data['files']:
            raise RuntimeError(f'Existing shell file is not managed: {name}')
    receipt.validate()
    with tempfile.TemporaryDirectory(prefix='sequoia-picker-update-') as temporary:
        stage = Path(temporary)
        destination = stage / shell.SHELL
        for name in widget_picker.FILES:
            (destination / name).parent.mkdir(parents=True, exist_ok=True)
        widget_picker.stage(destination, shell.SYSTEM_SHELL)
        if check:
            print('Widget-picker update validated; only four picker files will change.')
            return
        backup = prefix / 'sequoia-eleven/widget-picker-backups' / datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        backup.mkdir(parents=True)
        shutil.copy2(receipt.manifest, backup / 'manifest.json')
        for name in widget_picker.FILES:
            source = prefix / shell.SHELL / name
            if source.exists():
                target = backup / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
        receipt.install(stage, scope=lambda name: name in NAMES)
        receipt.validate()
        print(f'Installed centered widget picker. Previous picker and receipt: {backup}')
        print('Reload Plasma Shell to activate it.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    install(args.prefix.absolute(), args.check)
