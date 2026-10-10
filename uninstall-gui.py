#!/usr/bin/python3
"""Open the Sequoia Eleven uninstaller."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools'))

if __name__ == '__main__':
    from installer_bootstrap import launch
    launch('uninstaller_gui', Path(__file__).resolve(), 'Sequoia Eleven uninstaller')
