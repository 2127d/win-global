#!/usr/bin/env python3
"""Verify content sizing while switching directly between native tray widgets."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True, help='Directory for logs and screenshots')
parser.add_argument('--child', action='store_true', help=argparse.SUPPRESS)
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)

if args.child:
    if os.environ.get('SEQUOIA_TRAY_TEST_PRIVATE') != '1':
        raise SystemExit('The child must run inside the private test session.')
    work = Path(os.environ['SEQUOIA_TRAY_TEST_WORK'])
    compositor = None
    try:
        with (output / 'kwin.log').open('w') as log:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '900',
                                          '--no-lockscreen', '--no-global-shortcuts', '--socket', 'tray-sizing-test'],
                                         stdout=log, stderr=log)
        for _ in range(100):
            if (Path(os.environ['XDG_RUNTIME_DIR']) / 'tray-sizing-test').exists():
                break
            if compositor.poll() is not None:
                raise RuntimeError((output / 'kwin.log').read_text())
            time.sleep(.05)
        else:
            raise RuntimeError('The private compositor did not create its socket.')
        env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='tray-sizing-test',
                   LD_PRELOAD=str(work / 'tray-sizing-test.so'))
        with (output / 'native.log').open('w') as log:
            result = subprocess.run(['python3', str(work / 'stage/sequoia-eleven/control-center/launch.py'),
                                     '/usr/bin/plasmawindowed', 'org.kde.plasma.systemtray'],
                                    env=env, stdout=log, stderr=log, timeout=110)
        raise SystemExit(result.returncode)
    finally:
        if compositor:
            compositor.terminate()
            try:
                compositor.wait(timeout=3)
            except subprocess.TimeoutExpired:
                compositor.kill()
                compositor.wait()

for command in ('c++', 'pkg-config', 'kwin_wayland', 'plasmawindowed', 'dbus-run-session'):
    if not shutil.which(command):
        raise SystemExit(f'Missing test dependency: {command}')
with tempfile.TemporaryDirectory(prefix='sequoia-tray-sizing-test-', ignore_cleanup_errors=True) as temporary:
    work = Path(temporary)
    for kind in ('config', 'data', 'cache', 'state', 'run'):
        (work / kind).mkdir(mode=0o700)
    subprocess.run([sys.executable, str(ROOT / 'tools/install-control-center.py'), '--stage', '--prefix', str(work / 'stage')], check=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6DBus', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-std=c++17', '-fPIC', '-shared', str(ROOT / 'tools/test-support/tray-sizing-native.cpp'),
                    '-o', str(work / 'tray-sizing-test.so'), *flags], check=True)
    known = 'org.kde.plasma.vault,org.kde.kdeconnect,' + ','.join(p.stem for p in Path('/usr/lib64/qt6/plugins/plasma/applets').glob('*.so'))
    (work / 'config/plasmawindowedrc').write_text('[Applets][42]\nplugin=org.kde.plasma.systemtray\n' + ''.join(
        f'{group}\nknownItems={known}\nextraItems=org.kde.plasma.vault,org.kde.plasma.printmanager,org.kde.plasma.dwellclicker,org.kde.plasma.devicenotifier,org.kde.kdeconnect,org.kde.kscreen,org.kde.plasma.weather,org.kde.plasma.brightness,org.kde.plasma.battery,org.kde.plasma.notifications,org.kde.plasma.clipboard,org.kde.plasma.volume,org.kde.plasma.bluetooth,org.kde.plasma.networkmanagement\npin=true\n'
        for group in ('[Applets][42][General]', '[Applets][42][Configuration][General]')))
    variant = 'Light'
    (work / 'config/kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text())
    (work / 'config/plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant.lower()}\n')
    for name in ('plasma', 'icons'):
        (work / 'data' / name).symlink_to(ROOT / 'themes' / name, target_is_directory=True)
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic',
               QT_STYLE_OVERRIDE='Fusion', QT_FORCE_STDERR_LOGGING='1', SEQUOIA_VERIFY_DETAILS='weather,brightness,battery,notifications,clipboard,volume,bluetooth,networkmanagement,weather',
               SEQUOIA_TRAY_TEST_PRIVATE='1', SEQUOIA_TRAY_TEST_WORK=str(work),
               SEQUOIA_CONTROL_CAPTURE=str(output / 'control-center.png'), SEQUOIA_DETAIL_CAPTURE=str(output / 'tray-sizing'))
    for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE'):
        env[f'XDG_{kind}_HOME'] = str(work / kind.lower())
    env['XDG_RUNTIME_DIR'] = str(work / 'run')
    for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'DBUS_SESSION_BUS_ADDRESS', 'LD_PRELOAD', 'SEQUOIA_CONTROL_CENTER_DIR',
                 'SEQUOIA_VERIFY_DARK', 'SEQUOIA_VERIFY_LARGE', 'SEQUOIA_VERIFY_RTL'):
        env.pop(name, None)
    with (output / 'session.log').open('w') as log:
        result = subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()),
                                 '--output', str(output), '--child'], env=env, stdout=log, stderr=subprocess.STDOUT, timeout=125)
    native_log = output / 'native.log'
    print(native_log.read_text() if native_log.exists() else (output / 'session.log').read_text())
    raise SystemExit(result.returncode)
