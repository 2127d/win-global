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
TEST_WIDGET = 'plasma/plasmoids/org.sequoia.gridtest'
TEST_SOURCE = widget_picker.ROOT / 'widgets/plasmoids/org.sequoia.gridtest'
TEST_NAMES = {TEST_WIDGET + '/' + str(p.relative_to(TEST_SOURCE)) for p in TEST_SOURCE.rglob('*') if p.is_file()}


class PickerReceipt(Receipt):
    def validate(self, incoming=None, force=False, scope=None):
        # Other components may have intentional local edits. Leave their files
        # AND recorded hashes untouched; validate every file we actually change.
        if incoming is not None and not set(incoming) <= NAMES | TEST_NAMES:
            raise RuntimeError('Unexpected file in widget-picker payload.')
        super().validate(incoming, force, scope=lambda name: name in getattr(self, 'picker_names', NAMES))


def install(prefix, check=False, with_test_widget=False):
    names = NAMES | (TEST_NAMES if with_test_widget else set())
    receipt = PickerReceipt(prefix, installer.USER_STATE, installer.user_allowed)
    receipt.picker_names = names
    if 'plasma-style' not in shell.installed(receipt.data['files']):
        raise RuntimeError('Install the Sequoia Plasma style first with tools/install-theme.py.')
    for name in widget_picker.UPSTREAM_FILES:
        if shell.SHELL + '/' + name not in receipt.data['files']:
            raise RuntimeError(f'Existing shell file is not managed: {name}')
    receipt.validate()
    if with_test_widget and (prefix / TEST_WIDGET).exists() and not any(n in receipt.data['files'] for n in TEST_NAMES):
        raise RuntimeError('The test widget is separately installed; preserve it or remove it through Plasma first.')
    with tempfile.TemporaryDirectory(prefix='sequoia-picker-update-') as temporary:
        stage = Path(temporary)
        destination = stage / shell.SHELL
        for name in widget_picker.FILES:
            (destination / name).parent.mkdir(parents=True, exist_ok=True)
        widget_picker.stage(destination, shell.SYSTEM_SHELL)
        if with_test_widget:
            shutil.copytree(TEST_SOURCE, stage / TEST_WIDGET)
        if check:
            print('Widget picker and grid update validated; only tracked picker/grid files will change.')
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
        receipt.install(stage, scope=lambda name: name in names)
        receipt.validate()
        print(f'Installed widget picker and desktop grid. Previous picker and receipt: {backup}')
        print('Reload Plasma Shell to activate it.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--with-test-widget', action='store_true', help='Also install the adaptive grid test widget and track it for removal')
    args = parser.parse_args()
    install(args.prefix.absolute(), args.check, args.with_test_widget)
