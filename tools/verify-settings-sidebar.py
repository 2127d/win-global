#!/usr/bin/env python3
"""Build and exercise KDE's native settings sidebar against the staged style."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--scale', default='1')
args = parser.parse_args()
mode = 'dark' if args.dark else 'light'
with tempfile.TemporaryDirectory(prefix='sequoia-settings-sidebar-') as temporary:
    config = Path(temporary)
    helper = config / 'verify-settings-sidebar'
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-fPIC', str(ROOT / 'tools/verify-settings-sidebar.cpp'),
                    '-o', str(helper), '-I/usr/include/KF6/KWidgetsAddons',
                    *flags, '-lKF6WidgetsAddons'], check=True)
    (config / 'kdeglobals').write_text(
        (ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}Transparent.colors').read_text()
        + f'\n[Icons]\nTheme=SequoiaTahoe-{mode}\n')
    env = dict(os.environ, XDG_CONFIG_HOME=str(config), XDG_CACHE_HOME=str(config / 'cache'),
               XDG_DATA_HOME=str(config / 'data'), XDG_STATE_HOME=str(config / 'state'),
               QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='kde',
               QT_STYLE_OVERRIDE='Fusion', QT_SCALE_FACTOR=args.scale)
    output = ROOT / 'preview' / f'settings-sidebar-{mode}-{args.scale}.png'
    subprocess.run(['dbus-run-session', '--', str(helper),
                    str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'), str(output)],
                   env=env, check=True, timeout=30)
    print(f'Captured verified settings sidebar: {output}')
