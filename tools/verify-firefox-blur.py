#!/usr/bin/env python3
"""Check Firefox blur in a private Wayland compositor using synthetic windows."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--effects-build-root', type=Path,
                        help='Also load rebuilt Liquid Glass and modal dimming')
    parser.add_argument('--child', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    args.build_dir = args.build_dir.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    if args.child:
        with (args.output / 'kwin.log').open('w') as log:
            compositor = subprocess.Popen([
                'kwin_wayland', '--virtual', '--width', '1200', '--height', '800',
                '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities',
                '--inputmethod', '/bin/true', '--socket', 'firefox-test'],
                stdout=log, stderr=log)
            try:
                for _ in range(100):
                    if compositor.poll() is not None:
                        raise RuntimeError('Private compositor exited; inspect kwin.log')
                    if (Path(os.environ['XDG_RUNTIME_DIR']) / 'firefox-test').exists():
                        break
                    time.sleep(.1)
                else:
                    raise RuntimeError('Private compositor startup timed out')
                env = dict(os.environ, QT_QPA_PLATFORM='wayland',
                           WAYLAND_DISPLAY='firefox-test', FIREFOX_BLUR_CAPTURE=str(args.output))
                subprocess.run([str(args.build_dir / 'firefox-blur-probe')],
                               env=env, check=True, timeout=35)
            finally:
                compositor.terminate()
                try:
                    compositor.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    compositor.kill()
                    compositor.wait()
        return
    with tempfile.TemporaryDirectory(prefix='sq-ff-', ignore_cleanup_errors=True) as temporary:
        base = Path(temporary)
        env = dict(os.environ)
        for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH',
                    'QML2_IMPORT_PATH', 'QT_QUICK_CONTROLS_STYLE'):
            env.pop(key, None)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = base / kind.lower()
            path.mkdir(mode=0o700)
            env['XDG_' + kind + ('_DIR' if kind == 'RUNTIME' else '_HOME')] = str(path)
        plugins = base / 'plugins/kwin/effects/plugins'
        plugins.mkdir(parents=True)
        shutil.copy2(args.build_dir / 'sequoia_firefox_blur_v2.so', plugins)
        if args.effects_build_root:
            for component in ('liquid-glass', 'modal-dimming'):
                for plugin in (args.effects_build_root / component / 'lib/kwin/effects/plugins').glob('*.so'):
                    shutil.copy2(plugin, plugins)
            env['SEQUOIA_TEST_ALL_EFFECTS'] = '1'
        (base / 'config/kwinrc').write_text(
            '[Plugins]\nblurEnabled=false\ncontrastEnabled=false\nscaleEnabled=false\n'
            'fadingpopupsEnabled=false\n[Windows]\nPlacement=Centered\n')
        env.update(QT_FORCE_STDERR_LOGGING='1', QT_QPA_PLATFORM='offscreen',
                   QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                   QT_PLUGIN_PATH=str(base / 'plugins'), XDG_DATA_DIRS='/usr/local/share:/usr/share',
                   LIBGL_ALWAYS_SOFTWARE='1', KWIN_COMPOSE='O2',
                   KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1')
        subprocess.run(['dbus-run-session', '--', sys.executable, __file__, '--child',
                        '--build-dir', str(args.build_dir), '--output', str(args.output)],
                       env=env, check=True, timeout=50)


if __name__ == '__main__':
    main()
