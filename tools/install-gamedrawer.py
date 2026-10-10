#!/usr/bin/env python3
"""Inset the Game Drawer panel button background without resizing its hit area."""
import argparse
import os
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path,
                        default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    args = parser.parse_args()
    target = args.prefix / 'plasma/plasmoids/com.atopion.gamedrawer/contents/ui/main.qml'
    source = target.read_text()
    marker = '        // Sequoia Eleven: compact panel highlight.\n'
    if marker in source:
        print('Game Drawer already has the compact panel highlight.')
        return
    anchor = '    fullRepresentation: PlasmaComponents3.Button {\n'
    if source.count(anchor) != 1:
        raise RuntimeError('Game Drawer has changed; review its panel button before applying this adjustment.')
    result = source.replace(anchor, anchor + marker + '''        topInset: 6
        bottomInset: 6
        leftInset: 6
        rightInset: 6
''', 1)
    backup = target.with_suffix('.qml.before-compact-highlight')
    if not backup.exists():
        shutil.copy2(target, backup)
    target.write_text(result)
    print(f'Updated Game Drawer highlight: {target}')


if __name__ == '__main__':
    main()
