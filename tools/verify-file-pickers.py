#!/usr/bin/env python3
"""Exercise native and Qt choosers in all six Sequoia appearances, in isolation."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-plugins', type=Path, help='Previous plugins for pixel-exact non-picker regression checks')
parser.add_argument('--portal', action='store_true', help='Also exercise the real KDE desktop portal backend')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='sqp-') as temporary:
    root = Path(temporary)
    binary = root / 'verify-file-pickers'
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-w', '-fPIC', str(ROOT / 'tools/verify-file-pickers.cpp'), '-o', str(binary), *flags], check=True)
    hook = root / 'portal-probe.so'
    if args.portal:
        portal_flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Test', 'Qt6DBus'], text=True))
        subprocess.run(['c++', '-w', '-fPIC', '-shared', str(ROOT / 'tools/verify-file-picker-portal.cpp'), '-o', str(hook), *portal_flags], check=True)
    fixture = root / 'files'
    (fixture / 'Documents').mkdir(parents=True)
    (fixture / 'example.txt').write_text('File selection fixture.\n')
    (fixture / 'second.txt').write_text('Second file.\n')
    for mode in ('light', 'dark'):
        for material in ('solid', 'transparent', 'glass'):
            config = root / f'{mode}-{material}'
            config.mkdir()
            (config / 'runtime').mkdir(mode=0o700)
            style = 'SequoiaElevenGlass' if material == 'glass' else 'SequoiaEleven'
            colors = ROOT / f'themes/color-schemes/SequoiaEleven{mode.title()}{"Solid" if material == "solid" else "Transparent"}.colors'
            (config / 'kdeglobals').write_text(colors.read_text() + f'\n[KDE]\nwidgetStyle={style}\n[Icons]\nTheme=breeze\n')
            (config / 'kfiledialogrc').write_text('[KFileDialog Settings]\nView Style=DetailTree\nSpeedbar Width=220\n')
            output = ROOT / f'preview/file-pickers-{mode}-{material}'
            output.mkdir(parents=True, exist_ok=True)
            env = dict(os.environ, XDG_CONFIG_HOME=str(config), XDG_CACHE_HOME=str(config / 'cache'),
                       XDG_DATA_HOME=str(config / 'data'), XDG_STATE_HOME=str(config / 'state'),
                       XDG_RUNTIME_DIR=str(config / 'runtime'), QT_NO_XDG_DESKTOP_PORTAL='1',
                       QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE=style,
                       QT_FORCE_STDERR_LOGGING='1', QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
            env.pop('LD_PRELOAD', None)
            if args.baseline_plugins:
                baseline = config / 'baseline'
                baseline.mkdir()
                subprocess.run(['dbus-run-session', '--', str(binary), style, str(fixture), str(baseline), 'baseline'],
                               env=dict(env, QT_PLUGIN_PATH=str(args.baseline_plugins.resolve())), check=True, timeout=30, capture_output=True)
            result = subprocess.run(['dbus-run-session', '--', str(binary), style, str(fixture), str(output), 'verify'],
                                    env=env, timeout=40, capture_output=True, text=True)
            (output / 'verification.log').write_text(result.stdout + result.stderr)
            if result.returncode:
                raise SystemExit(f'{mode}-{material}: {result.stderr}')
            if args.baseline_plugins:
                for name in ('ordinary-window.png', 'ordinary-dialog.png'):
                    assert (output / name).read_bytes() == (baseline / name).read_bytes(), f'{mode}-{material}: non-picker changed: {name}'
            if args.portal:
                command = ['dbus-run-session', '--', 'env', f'LD_PRELOAD={hook}',
                    f'SEQUOIA_PICKER_PORTAL_CAPTURE={output / "portal-open.png"}',
                    f'SEQUOIA_PICKER_FIXTURE={fixture}', '/usr/libexec/xdg-desktop-portal-kde']
                log = output / 'portal-verification.log'
                with log.open('w') as stream:
                    process = subprocess.Popen(command, env=env, stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
                    try:
                        code = process.wait(timeout=20)
                    finally:
                        try:
                            os.killpg(process.pid, signal.SIGTERM)
                        except ProcessLookupError:
                            pass
                result = log.read_text()
                if code or 'PASS:' not in result:
                    raise SystemExit(f'{mode}-{material} portal: {result}')
            print(f'PASS {mode}-{material}: chooser behavior' + ('; ordinary windows pixel-identical' if args.baseline_plugins else ''), flush=True)
