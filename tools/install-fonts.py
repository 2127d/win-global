#!/usr/bin/env python3
"""Install bundled UI fonts and the Nunito display family for the current user."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path,
                        default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share'))
    args = parser.parse_args()
    for family in ['AdwaitaSans', 'Nunito']:
        source = ROOT/'fonts'/family
        manifest = json.loads((source/'sources.json').read_text())
        for name, record in manifest.items():
            if hashlib.sha256((source/name).read_bytes()).hexdigest() != record['sha256']:
                raise RuntimeError(f'Bundled font checksum mismatch: {name}')
        destination = args.prefix/'fonts/sequoia-eleven'/family
        destination.mkdir(parents=True, exist_ok=True)
        for name in [*manifest, 'sources.json']:
            shutil.copy2(source/name, destination/name)
        subprocess.run(['fc-cache', '-f', str(destination)], check=True)
        print(f'Installed {family} upright and italic variable fonts to {destination}')


if __name__ == '__main__':
    main()
