#!/usr/bin/env python3
"""Exercise standard/shared Quick menus under an isolated Wayland compositor."""
import argparse
import configparser
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--overlay', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
for material in ('solid', 'transparent', 'glass'):
    for mode in ('light', 'dark'):
        output = (args.output / f'{material}-{mode}').resolve()
        output.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='quick-menus-') as temporary:
            folder = Path(temporary)
            env = dict(os.environ)
            env.pop('QML2_IMPORT_PATH', None)
            env.pop('QT_QUICK_BACKEND', None)
            for kind in ('CONFIG', 'CACHE', 'STATE', 'RUNTIME'):
                path = folder / kind.lower()
                path.mkdir(mode=0o700)
                env[f'XDG_{kind}_HOME' if kind != 'RUNTIME' else 'XDG_RUNTIME_DIR'] = str(path)
            palette = ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}{"Solid" if material == "solid" else "Transparent"}.colors'
            colors = configparser.ConfigParser(interpolation=None)
            colors.optionxform = str
            colors.read(palette)
            if material == 'glass':
                for group in ('Colors:Window', 'Colors:Header'):
                    for key in ('BackgroundNormal', 'BackgroundAlternate'):
                        colors[group][key] = ','.join(colors[group][key].split(',')[:3]) + (',168' if mode == 'dark' else ',148')
            colors['Icons'] = {'Theme': 'breeze'}
            with (folder / 'config/kdeglobals').open('w') as handle:
                colors.write(handle, space_around_delimiters=False)
            (folder / 'config/kwinrc').write_text('[Plugins]\nblurEnabled=' + str(material != 'glass').lower()
                + '\nsequoia_liquid_glass_v6Enabled=' + str(material == 'glass').lower() + '\n')
            env.update(QML_IMPORT_PATH=str(args.overlay.resolve()), QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
                       QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='kde',
                       QT_STYLE_OVERRIDE='SequoiaEleven', QT_FORCE_STDERR_LOGGING='1', LIBGL_ALWAYS_SOFTWARE='1')
            command = ['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(ROOT / 'themes/application-style/quick/tests/tst_menus.qml'), '-o', str(output / 'tests.txt') + ',txt']
            launcher = folder / 'launch.sh'
            launcher.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexec ' + shlex.join(command) + '\n')
            launcher.chmod(0o755)
            with (output / 'compositor.log').open('w') as log:
                process = subprocess.Popen(['dbus-run-session', '--', 'kwin_wayland', '--virtual', '--width', '800', '--height', '700',
                    '--inputmethod', '/bin/true', '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities',
                    '--socket', 'menu-test', '--exit-with-session', str(launcher)], env=env, cwd=output,
                    stdout=log, stderr=log, start_new_session=True)
                try:
                    process.wait(timeout=45)
                finally:
                    try:
                        os.killpg(process.pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
                    process.wait(timeout=5)
            result = (output / 'tests.txt').read_text()
            assert '6 passed, 0 failed' in result, result
            log = (output / 'compositor.log').read_text()
            assert not any(error in log for error in ('TypeError:', 'ReferenceError:', 'Binding loop', 'Unable to assign')), log
            print(f'PASS: {material} {mode}: standard/shared menus, toggles, exclusive actions, disabled/hidden actions, submenus, RTL', flush=True)
