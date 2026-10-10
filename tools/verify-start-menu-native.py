#!/usr/bin/env python3
"""Run Start menu interaction tests under a private Wayland compositor."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def child(stage):
    env = dict(os.environ)
    for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD'):
        env.pop(key, None)
    for name in ('config', 'data', 'cache', 'run', 'state'):
        (stage / name).mkdir(mode=0o700, exist_ok=True)
    env.update(XDG_CONFIG_HOME=str(stage / 'config'), XDG_DATA_HOME=str(stage / 'data'),
               XDG_CACHE_HOME=str(stage / 'cache'), XDG_RUNTIME_DIR=str(stage / 'run'),
               XDG_STATE_HOME=str(stage / 'state'), QT_QPA_PLATFORM='offscreen',
               QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic',
               # The Python harness needs the positioning protocol normally
               # restricted to plasmashell. This affects only this private KWin.
               KWIN_WAYLAND_NO_PERMISSION_CHECKS='1', QT_NO_XDG_DESKTOP_PORTAL='1')
    compositor = None
    try:
        with (stage / 'kwin.log').open('w') as log:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1920',
                '--height', '1080', '--no-lockscreen', '--no-global-shortcuts',
                '--socket', 'start-menu-test'], env=env, stdout=log, stderr=log)
        for _ in range(100):
            if (stage / 'run/start-menu-test').exists():
                break
            if compositor.poll() is not None:
                raise RuntimeError((stage / 'kwin.log').read_text())
            time.sleep(.1)
        else:
            raise RuntimeError('Private compositor failed to start')
        env.update(SEQUOIA_START_TEST_PLATFORM='wayland', WAYLAND_DISPLAY='start-menu-test',
                   WAYLAND_DEBUG='client', PYTHONUNBUFFERED='1',
                   # Match plasmashell's Qt Wayland position bookkeeping.
                   QT_WAYLAND_DISABLE_FIXED_POSITIONS='')
        with (stage / 'client.log').open('w') as log:
            result = subprocess.run([sys.executable, str(ROOT / 'tools/verify-start-menu.py'),
                '--child', str(stage)], env=env, stdout=log, stderr=log, timeout=60)
        output = (stage / 'client.log').read_text()
        print('\n'.join(line for line in output.splitlines() if line.startswith('PASS:')))
        if result.returncode:
            print(output[-16000:], file=sys.stderr)
            raise RuntimeError('Native Start menu checks failed')
        errors = [line for line in output.splitlines() if 'TypeError:' in line or 'ReferenceError:' in line]
        assert not errors, '\n'.join(errors)
        blur_requests = [line for line in output.splitlines()
                         if ('ext_background_effect_surface_v1#' in line
                             and '.set_blur_region(' in line and 'nil' not in line)
                         or ('org_kde_kwin_blur#' in line and '.commit(' in line)]
        assert len(blur_requests) >= 3, ('Native menu windows did not submit compositor blur regions\n' +
            '\n'.join(line for line in output.splitlines() if 'blur' in line or 'background_effect' in line))
        print('PASS: native Wayland popup interaction and compositor blur-region requests')

    finally:
        if compositor and compositor.poll() is None:
            compositor.terminate()
            try:
                compositor.wait(timeout=5)
            except subprocess.TimeoutExpired:
                compositor.kill()
                compositor.wait()


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--child':
        child(Path(sys.argv[2]))
    else:
        with tempfile.TemporaryDirectory(prefix='sequoia-start-native-') as directory:
            # The daemon and its activated services must also use private paths.
            stage = Path(directory)
            env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
            for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE'):
                path = stage / kind.lower()
                path.mkdir(mode=0o700)
                env[f'XDG_{kind}_HOME'] = str(path)
            (stage / 'run').mkdir(mode=0o700)
            env['XDG_RUNTIME_DIR'] = str(stage / 'run')
            subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()),
                            '--child', directory], env=env, check=True, timeout=90)
