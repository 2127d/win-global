"""Maintain Liquid Glass palettes. The broken Global Theme wrappers are retired."""
import configparser
import io
import os
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build(root=ROOT):
    for mode, alpha in (('light', 148), ('dark', 168)):
        name = f'Sequoia Eleven Liquid Glass {mode.title()}'
        scheme = f'SequoiaEleven{mode.title()}Glass'
        palette = configparser.ConfigParser(interpolation=None)
        palette.optionxform = str
        palette.read(root / f'themes/color-schemes/SequoiaEleven{mode.title()}Transparent.colors')
        for group in ('Colors:Window', 'Colors:Header'):
            for key in ('BackgroundNormal', 'BackgroundAlternate'):
                palette[group][key] = ','.join(palette[group][key].split(',')[:3] + [str(alpha)])
        for key in ('activeBackground', 'inactiveBackground'):
            palette['WM'][key] = ','.join(palette['WM'][key].split(',')[:3] + [str(alpha)])
        palette['General']['Name'] = name
        palette['General']['ColorScheme'] = scheme
        output = io.StringIO()
        palette.write(output, space_around_delimiters=False)
        (root / 'themes/color-schemes' / f'{scheme}.colors').write_text(output.getvalue())


def install():
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
    for mode in ('light', 'dark'):
        scheme = f'SequoiaEleven{mode.title()}Glass.colors'
        (data / 'color-schemes').mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'themes/color-schemes' / scheme, data / 'color-schemes' / scheme)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', action='store_true', help='Install only the Liquid Glass palettes, without applying them or creating Global Themes.')
    args = parser.parse_args()
    build()
    if args.install:
        install()
