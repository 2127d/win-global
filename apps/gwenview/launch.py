#!/usr/bin/env python3
"""Launch the matching Gwenview override, falling back after a package upgrade."""
import hashlib
import json
import os
from pathlib import Path
import sys

INSTALL_DATA = Path('__SEQUOIA_GWENVIEW_DATA__')
STOCK = Path('/usr/bin/gwenview')
STOCK_LIBRARY = Path('/usr/lib64/libgwenviewlib.so.5')


def command(data, stock, library, arguments):
    target = data / 'sequoia-eleven/gwenview'
    binary = target / 'gwenview'
    try:
        manifest = json.loads((target / 'build.json').read_text())
        valid = (binary.is_file() and os.access(binary, os.X_OK)
                 and (target / 'libgwenviewlib.so.5').is_file()
                 and manifest['upstream_sha256'] == hashlib.sha256(stock.read_bytes()).hexdigest()
                 and manifest['upstream_library_sha256'] == hashlib.sha256(library.read_bytes()).hexdigest())
    except (OSError, ValueError, KeyError):
        valid = False
    if not valid:
        print('The Gwenview override needs rebuilding. Opening the installed KDE version.', file=sys.stderr)
    return [str(binary if valid else stock), *arguments]


if __name__ == '__main__':
    argv = command(INSTALL_DATA, STOCK, STOCK_LIBRARY, sys.argv[1:])
    os.execv(argv[0], argv)
