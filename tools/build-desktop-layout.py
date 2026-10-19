#!/usr/bin/env python3
"""Bundle the reviewed desktop preset as a standalone script in every Global Theme."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = 'contents/layouts/org.kde.plasma.desktop-layout.js'


def rendered():
    layout = json.loads((ROOT / 'desktop/layouts/desktop-layout.json').read_text())
    template = (ROOT / 'desktop/layouts/desktop-layout.js.in').read_text()
    return template.replace('@PANELS@', json.dumps(layout['panels'], indent=4))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    script = rendered()
    for theme in sorted((ROOT / 'themes/plasma/look-and-feel').glob('org.sequoia.eleven.*')):
        target = theme / SCRIPT
        if args.check:
            if not target.is_file() or target.read_text() != script:
                raise SystemExit(f'Outdated layout: {target}; run tools/build-desktop-layout.py')
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(script)
    print('Global Theme desktop layouts are up to date.')


if __name__ == '__main__':
    main()
