#!/usr/bin/env python3
"""Remove a reviewed selection through the receipt-based installers."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

from installer_platform import APPS, COMPONENTS, resolve_removals, run

ROOT = Path(__file__).resolve().parents[1]


def add_destinations(parser):
    parser.add_argument('--prefix', type=Path,
                        default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--system-root', type=Path, default=Path('/'))
    parser.add_argument('--bin-prefix', type=Path, default=Path.home() / '.local/bin')


def removal_commands(args):
    steps = []
    components = resolve_removals(args.components)
    if components:
        command = [sys.executable, '-u', str(ROOT / 'tools/install-theme.py'), 'uninstall',
                   '--prefix', str(args.prefix), '--system-root', str(args.system_root),
                   '--components', *components]
        if args.keep_system:
            command.append('--keep-system')
        steps.append(('Selected theme components', command))
    for app in args.apps:
        script = {'settings': 'install-settings-navigation.py', 'gwenview': 'install-gwenview.py', 'kcalc': 'install-kcalc.py', 'kweather': 'install-kweather.py'}[app]
        steps.append((APPS[app][0], [sys.executable, '-u', str(ROOT / 'tools' / script), '--remove',
                                   '--prefix', str(args.prefix), '--bin-prefix', str(args.bin_prefix)]))
    if args.force:
        for _, command in steps:
            command.append('--force')
    return steps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    add_destinations(parser)
    parser.add_argument('--components', nargs='*', choices=COMPONENTS, default=[])
    parser.add_argument('--apps', nargs='*', choices=APPS, default=[])
    parser.add_argument('--keep-system', action='store_true')
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--plan', action='store_true')
    args = parser.parse_args()
    if not args.components and not args.apps:
        parser.error('Select at least one component or app makeover to remove.')
    steps = removal_commands(args)
    if args.plan:
        for title, command in steps:
            print(title + ': ' + ' '.join(command))
        return
    if os.geteuid() == 0:
        raise RuntimeError('Open the uninstaller as your desktop user, without sudo.')
    # Check every selected receipt before removing any theme or app files.
    print('STEP: Checking selected installations', flush=True)
    for _, command in steps:
        run([*command, '--check'])
    for title, command in steps:
        print('STEP: Removing ' + title, flush=True)
        run(command)
        print('COMPLETED: ' + title, flush=True)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.SubprocessError) as error:
        sys.exit(f'Removal stopped: {error}')
