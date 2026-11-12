"""Generate the native KDE light/dark Global Themes for installed Liquid Glass."""
import configparser
import io
import json
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build(root=ROOT):
    for mode, alpha in (('light', 148), ('dark', 168)):
        name = f'Sequoia Eleven Liquid Glass {mode.title()}'
        scheme = f'SequoiaEleven{mode.title()}Glass'
        package_id = f'org.sequoia.eleven.{mode}.glass'
        package = root / 'themes/plasma/look-and-feel' / package_id
        (package / 'contents').mkdir(parents=True, exist_ok=True)
        shutil.copytree(root / f'themes/plasma/look-and-feel/org.sequoia.eleven.{mode}/contents/splash',
                        package / 'contents/splash', dirs_exist_ok=True)
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
        # Embed the palette so KDE can classify the package in the Light/Dark
        # pickers even when the standalone color scheme has not been installed.
        (package / 'contents/colors').write_text(output.getvalue())
        (package / 'metadata.json').write_text(json.dumps({
            'KPackageStructure': 'Plasma/LookAndFeel',
            'KPlugin': {'Id': package_id, 'Name': name,
                        'Description': f'{mode.title()} Liquid Glass application surfaces, window decorations, and Plasma style.',
                        'Version': '2.0.0', 'License': 'MIT'},
            'X-Plasma-APIVersion': '2',
        }, indent=2) + '\n')
        # Native Quick Settings applies these through KLookAndFeelManager.
        # Do not include a desktop layout or overwrite the user's geometry,
        # fonts, wallpaper, rim settings, or compositor configuration.
        (package / 'contents/defaults').write_text(f'''[kdeglobals][KDE]
widgetStyle=SequoiaElevenGlass

[kdeglobals][General]
ColorScheme={scheme}

[kdeglobals][Icons]
Theme=SequoiaTahoe-{mode}

[plasmarc][Theme]
name=sequoia-eleven-glass-{mode}

[kwinrc][org.kde.kdecoration2]
library=org.kde.sequoiaeleven.glass-adjustable

[ksplashrc][KSplash]
Engine=KSplashQML
Theme={package_id}
''')


def install():
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
    plugins = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
    required = [plugins / 'styles/sequoiaeleven_glass6.so',
                plugins / 'org.kde.kdecoration3/org.kde.sequoiaeleven.glass-adjustable.so']
    required += [data / f'plasma/desktoptheme/sequoia-eleven-glass-{mode}/metadata.json' for mode in ('light', 'dark')]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise RuntimeError('Install the Liquid Glass components first: ' + ', '.join(missing))
    for mode in ('light', 'dark'):
        package_id = f'org.sequoia.eleven.{mode}.glass'
        source = ROOT / 'themes/plasma/look-and-feel' / package_id
        destination = data / 'plasma/look-and-feel' / package_id
        action = '--upgrade' if destination.exists() else '--install'
        subprocess.run(['kpackagetool6', '--type', 'Plasma/LookAndFeel', action, str(source)], check=True)
        scheme = f'SequoiaEleven{mode.title()}Glass.colors'
        (data / 'color-schemes').mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'themes/color-schemes' / scheme, data / 'color-schemes' / scheme)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', action='store_true', help='Install the pair for an existing Liquid Glass setup without applying it.')
    args = parser.parse_args()
    build()
    if args.install:
        install()
