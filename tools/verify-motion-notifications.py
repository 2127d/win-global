#!/usr/bin/env python3
"""Verify live animation-speed notifications in an isolated KWin session."""
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

import motion_install as motion


def child(base):
    config = base / 'config'
    log_path = base / 'kwin.log'
    with log_path.open('w') as log:
        compositor = subprocess.Popen([
            'kwin_wayland', '--virtual', '--width', '800', '--height', '600',
            '--inputmethod', '/bin/true', '--no-lockscreen', '--no-global-shortcuts',
            '--no-kactivities', '--socket', 'motion-notification-test',
        ], stdout=log, stderr=log, start_new_session=True)
    try:
        for _ in range(100):
            if compositor.poll() is not None:
                raise RuntimeError(log_path.read_text())
            try:
                if motion.effect_call('isEffectLoaded', 'overview') == 'true':
                    break
            except subprocess.CalledProcessError:
                pass
            time.sleep(.1)
        else:
            raise RuntimeError('Private KWin did not load Overview: ' + log_path.read_text())

        def duration():
            info = motion.effect_call('supportInformation', 'overview')
            return next(int(line.split(':', 1)[1]) for line in info.splitlines()
                        if line.startswith('animationDuration:'))

        def write(value):
            motion.run('kwriteconfig6', '--file', str(config / 'kdeglobals'),
                       '--group', 'KDE', '--key', 'AnimationDurationFactor', '--notify', value)

        def verify(expected):
            for _ in range(30):
                motion.effect_call('reconfigureEffect', 'overview')
                actual = duration()
                if actual == expected:
                    return
                time.sleep(.1)
            raise AssertionError(f'Overview remained at {actual} ms; expected {expected} ms')

        verify(300)
        # Reproduce the original absolute-path write and notification pulse.
        write(str(2 / 3 + .000000001))
        write(str(2 / 3))
        motion.run('qdbus-qt6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.reconfigure')
        time.sleep(.2)
        motion.effect_call('reconfigureEffect', 'overview')
        assert duration() == 300, 'Expected the absolute-path notification regression'
        print('Reproduced: saved factor 2/3, but absolute-path notification leaves Overview at 300 ms.', flush=True)

        for factor, expected in ((2 / 3, 200), (2 / 3, 200), (1.5, 450), (None, 300)):
            write(str(factor) if factor is not None else '--delete')
            motion.notify_factor(config)
            verify(expected)
            value = motion.run('kreadconfig6', '--file', str(config / 'kdeglobals'),
                               '--group', 'KDE', '--key', 'AnimationDurationFactor')
            assert value == (str(factor) if factor is not None else ''), value
            print(f'PASS: notification updates running Overview to {expected} ms; saved value preserved.', flush=True)
    finally:
        if compositor.poll() is None:
            os.killpg(compositor.pid, signal.SIGTERM)
            try:
                compositor.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(compositor.pid, signal.SIGKILL)
                compositor.wait()


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--child':
        child(Path(sys.argv[2]))
        return
    with tempfile.TemporaryDirectory(prefix='sequoia-motion-notifications-') as temporary:
        base = Path(temporary)
        env = dict(os.environ)
        for name in ('CONFIG', 'DATA', 'CACHE', 'STATE', 'RUNTIME'):
            path = base / name.lower()
            path.mkdir(mode=0o700)
            env['XDG_RUNTIME_DIR' if name == 'RUNTIME' else f'XDG_{name}_HOME'] = str(path)
        for name in ('LD_PRELOAD', 'DISPLAY', 'WAYLAND_DISPLAY', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH'):
            env.pop(name, None)
        env.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                   QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion', LIBGL_ALWAYS_SOFTWARE='1')
        (base / 'config/kdeglobals').write_text('[KDE]\nAnimationDurationFactor=1\n')
        (base / 'config/kwinrc').write_text('[Plugins]\noverviewEnabled=true\n')
        bus = base / 'bus.conf'
        bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir=' + str(base / 'runtime')
                       + '</listen><policy context="default"><allow send_destination="*"/>'
                       '<allow receive_sender="*"/><allow own="*"/></policy></busconfig>')
        subprocess.run(['dbus-run-session', '--config-file=' + str(bus), '--', sys.executable,
                        str(Path(__file__).resolve()), '--child', str(base)], env=env, check=True, timeout=60)


if __name__ == '__main__':
    main()
