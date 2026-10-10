#!/usr/bin/env python3
"""Launch the matching KCalc override, falling back after a package upgrade."""
import hashlib
import json
import os
from pathlib import Path
import sys

INSTALL_DATA = Path('__SEQUOIA_KCALC_DATA__')
STOCK = Path('/usr/bin/kcalc')


def command(data, stock, arguments):
    target = data / 'sequoia-eleven/kcalc'
    binary = target / 'kcalc'
    try:
        manifest = json.loads((target / 'build.json').read_text())
        valid = (binary.is_file() and os.access(binary, os.X_OK)
                 and manifest['upstream_sha256'] == hashlib.sha256(stock.read_bytes()).hexdigest()
                 and manifest['binary_sha256'] == hashlib.sha256(binary.read_bytes()).hexdigest())
    except (OSError, ValueError, KeyError):
        valid = False
    if not valid:
        print('The KCalc override needs rebuilding. Opening the installed KDE version.', file=sys.stderr)
    return [str(binary if valid else stock), *arguments]


if __name__ == '__main__':
    argv = command(INSTALL_DATA, STOCK, sys.argv[1:])
    os.execv(argv[0], argv)
