#!/usr/bin/env python3
"""Verify the effect in an isolated Wayland compositor; never touches the live desktop."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/modal-build')
    parser.add_argument('--child', action='store_true')
    parser.add_argument('--decoration', default='org.kde.breeze')
    parser.add_argument('--native-parent', action='store_true', help='Keep KDE dialogparent enabled to check coexistence')
    parser.add_argument('--reduced-motion', action='store_true')
    args = parser.parse_args()
    output = ROOT / 'preview/modal-dimming' / (args.decoration + ('-native-parent' if args.native_parent else '') + ('-reduced-motion' if args.reduced_motion else ''))
    output.mkdir(parents=True, exist_ok=True)
    if args.child:
        with (output / 'kwin.log').open('w') as log:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '800',
                '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities', '--inputmethod', '/bin/true',
                '--socket', 'sequoia-modal-test'], stdout=log, stderr=log)
            try:
                for _ in range(100):
                    if compositor.poll() is not None: raise RuntimeError('Private KWin exited')
                    if (Path(os.environ['XDG_RUNTIME_DIR']) / 'sequoia-modal-test').exists(): break
                    time.sleep(.1)
                env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-modal-test', MODAL_CAPTURE=str(output))
                subprocess.run([str(args.build_dir / 'modal-dimming-probe')], env=env, check=True, timeout=35)
            finally:
                compositor.terminate()
                try: compositor.wait(timeout=5)
                except subprocess.TimeoutExpired: compositor.kill(); compositor.wait()
        return
    with tempfile.TemporaryDirectory(prefix='sequoia-modal-test-', ignore_cleanup_errors=True) as temporary:
        base = Path(temporary)
        env = dict(os.environ)
        for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH', 'QT_QUICK_CONTROLS_STYLE'):
            env.pop(name, None)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = base / kind.lower(); path.mkdir(mode=0o700)
            env['XDG_' + kind + ('_DIR' if kind == 'RUNTIME' else '_HOME')] = str(path)
        (base / 'config/kwinrc').write_text('[Plugins]\nsequoia_modal_dimmingEnabled=true\ndimparentEnabled=false\ndialogparentEnabled=false\ndiminactiveEnabled=false\nblurEnabled=false\ncontrastEnabled=false\nscaleEnabled=false\nfadingpopupsEnabled=false\n\n[org.kde.kdecoration2]\nlibrary=' + args.decoration + '\n')
        if args.native_parent:
            with (base / 'config/kwinrc').open('a') as stream: stream.write('[Plugins]\ndialogparentEnabled=true\n')
        if args.reduced_motion:
            (base / 'config/kdeglobals').write_text('[KDE]\nAnimationDurationFactor=0\n')
        env.update(QT_FORCE_STDERR_LOGGING='1', QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                   QT_PLUGIN_PATH=str(args.build_dir / 'lib'), XDG_DATA_DIRS='/usr/local/share:/usr/share',
                   LIBGL_ALWAYS_SOFTWARE='1', KWIN_COMPOSE='O2', KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1')
        subprocess.run(['dbus-run-session', '--', sys.executable, __file__, '--child', '--build-dir', str(args.build_dir),
                        '--decoration', args.decoration, *(['--native-parent'] if args.native_parent else []),
                        *(['--reduced-motion'] if args.reduced_motion else [])], env=env, check=True, timeout=50)

if __name__ == '__main__': main()
