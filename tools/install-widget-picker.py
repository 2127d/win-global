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
GRID_WIDGETS = set(widget_picker.catalog()) & installer.LAYOUT_WIDGETS


def widget_files(plugins):
    return {f'plasma/plasmoids/{plugin}/{p.relative_to(source)}'
            for plugin in plugins
            for source in [widget_picker.ROOT / 'widgets/plasmoids' / plugin]
            for p in source.rglob('*') if p.is_file()}


class PickerReceipt(Receipt):
    def validate(self, incoming=None, force=False, scope=None):
        # Other components may have intentional local edits. Leave their files
        # AND recorded hashes untouched; validate every file we actually change.
        if incoming is not None and not set(incoming) <= NAMES | widget_files(GRID_WIDGETS):
            raise RuntimeError('Unexpected file in widget-picker payload.')
        super().validate(incoming, force, scope=lambda name: name in getattr(self, 'picker_names', NAMES))


def install(prefix, check=False, with_test_widget=False, with_widgets=()):
    plugins = set(with_widgets) | ({'org.sequoia.gridtest'} if with_test_widget else set())
    if not plugins <= GRID_WIDGETS:
        raise RuntimeError('Only managed grid widgets can be installed with the picker.')
    names = NAMES | widget_files(plugins)
    receipt = PickerReceipt(prefix, installer.USER_STATE, installer.user_allowed)
    receipt.picker_names = names
    if 'plasma-style' not in shell.installed(receipt.data['files']):
        raise RuntimeError('Install the Sequoia Plasma style first with tools/install-theme.py.')
    for name in widget_picker.UPSTREAM_FILES:
        if shell.SHELL + '/' + name not in receipt.data['files']:
            raise RuntimeError(f'Existing shell file is not managed: {name}')
    receipt.validate()
    for plugin in plugins:
        if (prefix / 'plasma/plasmoids' / plugin).exists() and not any(n in receipt.data['files'] for n in widget_files({plugin})):
            raise RuntimeError(f'The widget {plugin} is separately installed; preserve it or remove it through Plasma first.')
    with tempfile.TemporaryDirectory(prefix='sequoia-picker-update-') as temporary:
        stage = Path(temporary)
        destination = stage / shell.SHELL
        for name in widget_picker.FILES:
            (destination / name).parent.mkdir(parents=True, exist_ok=True)
        widget_picker.stage(destination, shell.SYSTEM_SHELL)
        for plugin in plugins:
            shutil.copytree(widget_picker.ROOT / 'widgets/plasmoids' / plugin, stage / 'plasma/plasmoids' / plugin)
        if check:
            print('Widget picker and grid update validated; only picker/grid files and explicitly selected widgets will change.')
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
        for name in widget_files(plugins):
            if (prefix / name).exists():
                target = backup / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(prefix / name, target)
        receipt.install(stage, scope=lambda name: name in names)
        receipt.validate()
        print(f'Installed widget picker and desktop grid. Previous picker and receipt: {backup}')
        print('Reload Plasma Shell to activate it.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--with-test-widget', action='store_true', help='Also install the adaptive grid test widget and track it for removal')
    parser.add_argument('--with-widget', action='append', choices=sorted(GRID_WIDGETS), default=[], help='Also install this grid widget and track it for removal')
    args = parser.parse_args()
    install(args.prefix.absolute(), args.check, args.with_test_widget, args.with_widget)
