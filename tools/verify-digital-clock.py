#!/usr/bin/env python3
"""Validate and capture Sequoia Digital Clock using private XDG paths and D-Bus."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.sequoia.digitalclock'


def native_child(library):
    compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '900',
                                   '--no-lockscreen', '--no-global-shortcuts', '--socket', 'sequoia-clock-test'])
    try:
        socket = Path(os.environ['XDG_RUNTIME_DIR']) / 'sequoia-clock-test'
        for _ in range(100):
            if socket.exists():
                break
            if compositor.poll() is not None:
                raise RuntimeError('Private clock compositor exited')
            time.sleep(.1)
        assert socket.exists(), 'Private clock compositor did not start'
        env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-clock-test',
                   QT_QUICK_BACKEND='rhi', QSG_RHI_BACKEND='opengl', LIBGL_ALWAYS_SOFTWARE='1', LD_PRELOAD=library)
        return subprocess.run(['plasmawindowed', PLUGIN], env=env, timeout=25).returncode
    finally:
        compositor.terminate()
        try:
            compositor.wait(timeout=5)
        except subprocess.TimeoutExpired:
            compositor.kill()
            compositor.wait()


def main():
    subprocess.run([sys.executable, str(ROOT / 'tools/verify-digital-clock-compatibility.py')], check=True)
    library = ROOT / '.staging/verify-digital-clock.so'
    library.parent.mkdir(exist_ok=True)
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-std=c++17', '-fPIC', '-shared',
                    str(ROOT / 'tools/verify-digital-clock.cpp'), '-o', str(library), *flags], check=True)
    for mode in ('light', 'dark'):
        with tempfile.TemporaryDirectory(prefix='sequoia-clock-', ignore_cleanup_errors=True) as temporary:
            stage = Path(temporary)
            env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                       QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion', QT_NO_XDG_DESKTOP_PORTAL='1', QT_FORCE_STDERR_LOGGING='1',
                       SEQUOIA_CLOCK_CAPTURE=str(ROOT / f'preview/digital-clock-{mode}'))
            for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH', 'QT_PLUGIN_PATH'):
                env.pop(key, None)
            for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
                directory = stage / kind.lower()
                directory.mkdir(mode=0o700)
                env[f'XDG_{kind}_' + ('DIR' if kind == 'RUNTIME' else 'HOME')] = str(directory)
            shutil.copytree(ROOT / f'plasma/plasmoids/{PLUGIN}', stage / f'data/plasma/plasmoids/{PLUGIN}')
            shutil.copytree(ROOT / f'plasma/desktoptheme/sequoia-eleven-{mode}',
                            stage / f'data/plasma/desktoptheme/sequoia-eleven-{mode}')
            (stage / 'config/plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{mode}\n')
            shutil.copy2(ROOT / f'color-schemes/SequoiaEleven{mode.title()}.colors', stage / 'config/kdeglobals')
            (stage / 'data/icons').symlink_to(ROOT / 'icons')
            with (stage / 'config/kdeglobals').open('a') as config:
                config.write(f'\n[Icons]\nTheme=SequoiaTahoe{"-dark" if mode == "dark" else ""}\n')
            logfile = ROOT / f'preview/digital-clock-{mode}.log'
            command = (['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()), '--native-child', str(library)]
                       if '--native' in sys.argv else
                       ['dbus-run-session', '--', 'env', f'LD_PRELOAD={library}', 'plasmawindowed', PLUGIN])
            with logfile.open('w') as log:
                result = subprocess.run(command, env=env, stdout=log, stderr=log, timeout=40)
            output = logfile.read_text()
            errors = [line for line in output.splitlines() if any(word in line for word in
                      ('ReferenceError:', 'TypeError:', 'Cannot assign', 'is not a type', 'unavailable'))]
            if result.returncode or errors or 'PASS:' not in output:
                raise RuntimeError(output)
            print('\n'.join(mode + ': ' + line for line in output.splitlines() if line.startswith('PASS:')))


if __name__ == '__main__':
    if '--native-child' in sys.argv:
        raise SystemExit(native_child(sys.argv[-1]))
    main()
