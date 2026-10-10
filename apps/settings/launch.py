#!/usr/bin/env python3
"""Launch the matching navigation build, falling back to KDE after an upgrade."""
import hashlib
import json
import os
from pathlib import Path
import sys

# Replaced with the chosen data directory by the installer, using Python repr.
INSTALL_DATA = Path('__SEQUOIA_SETTINGS_DATA__')
STOCK = Path('/usr/bin/systemsettings')


def command(data, stock, arguments):
    target = data / 'sequoia-eleven/settings-navigation'
    binary = target / 'systemsettings'
    try:
        manifest = json.loads((target / 'build.json').read_text())
        valid = binary.is_file() and os.access(binary, os.X_OK) and manifest['upstream_sha256'] == hashlib.sha256(stock.read_bytes()).hexdigest()
    except (OSError, ValueError, KeyError):
        valid = False
    executable = binary if valid else stock
    if not valid:
        print('The custom Settings build needs refreshing. Opening the installed KDE version.', file=sys.stderr)
    quick = data / 'sequoia-eleven/quick-islands/sequoia-quick'
    # The existing desktop launcher already supplies the theme. Terminal launches
    # can use it too; do not nest it when an overlay has already been selected.
    if quick.is_file() and not os.environ.get('QML_IMPORT_PATH'):
        return [str(quick), str(executable), *arguments]
    return [str(executable), *arguments]


if __name__ == '__main__':
    argv = command(INSTALL_DATA, STOCK, sys.argv[1:])
    os.execv(argv[0], argv)
