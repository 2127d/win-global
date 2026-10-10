#!/usr/bin/env python3
"""Exercise the actual tray in a private D-Bus/config session and capture it."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--debug', action='store_true')
parser.add_argument('--platform', choices=['wayland', 'offscreen'], default='wayland')
parser.add_argument('--graphics', action='store_true', help='Use the scene graph renderer to inspect panel shadows')
parser.add_argument('--media', action='store_true', help='Check the redesigned native media popup')
parser.add_argument('--reuse-bus', action='store_true', help='Use the caller’s isolated test bus')
parser.add_argument('--details', action='store_true', help='Open and capture every redesigned detail menu')
parser.add_argument('--contacts', action='store_true', help='Check Merkuro Contacts and nested Back navigation')
parser.add_argument('--contacts-startup', action='store_true', help='Check login stays responsive without starting Akonadi')
parser.add_argument('--dark', action='store_true')
parser.add_argument('--large', action='store_true')
parser.add_argument('--rtl', action='store_true')
args = parser.parse_args()
(ROOT / '.staging').mkdir(exist_ok=True)
(ROOT / 'preview').mkdir(exist_ok=True)
helper = ROOT / '.staging/verify-control-center.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6DBus'], text=True))
subprocess.run(['c++', '-std=c++17', '-fPIC', '-shared', str(ROOT / 'tools/verify-control-center.cpp'), '-o', str(helper), *flags], check=True)
directory = ROOT / '.staging/control-center-data/sequoia-eleven/control-center'
with tempfile.TemporaryDirectory(prefix='sequoia-native-tray-', ignore_cleanup_errors=True) as temporary:
    known = ','.join(path.stem for path in Path('/usr/lib64/qt6/plugins/plasma/applets').glob('*.so')) + ',org.kde.kdeconnect,org.kde.plasma.vault'
    enabled = ','.join('org.kde.plasma.' + name for name in ('volume', 'brightness', 'bluetooth', 'networkmanagement', 'notifications', 'mediacontroller', 'battery', 'clipboard'))
    if args.details or args.contacts or args.contacts_startup:
        from control_center_details import APPLETS
        APPLETS = ('contacts',) if args.contacts or args.contacts_startup else (*APPLETS, 'weather', 'contacts')
        enabled = ','.join('org.kde.merkuro.contact.applet' if name == 'contacts' else 'org.kde.plasma.' + name for name in APPLETS)
    settings = '[Applets][42]\nplugin=org.kde.plasma.systemtray\n'
    for group in ('[Applets][42][General]', '[Applets][42][Configuration][General]'):
        settings += f'{group}\nknownItems={known}\nextraItems={enabled}\npin=true\n'
    (Path(temporary) / 'plasmawindowedrc').write_text(settings)
    variant = 'Dark' if args.dark else 'Light'
    (Path(temporary) / 'kdeglobals').write_text(
        (ROOT / f'color-schemes/SequoiaEleven{variant}Solid.colors').read_text()
        + f'\n[General]\nfont=Adwaita Sans,{13 if args.large else 10},-1,5,400,0,0,0,0,0\n')
    (Path(temporary) / 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant.lower()}\n')
    (Path(temporary) / 'plasma').symlink_to(ROOT / 'plasma', target_is_directory=True)
    (Path(temporary) / 'icons').symlink_to(ROOT / 'icons', target_is_directory=True)
    (Path(temporary) / 'kded6rc').write_text('[Module-kded_plasma_welcome]\nautoload=false\n')
    env = dict(os.environ, QT_QPA_PLATFORM=args.platform, QT_QUICK_BACKEND='rhi' if args.graphics else 'software',
               QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
               QT_FORCE_STDERR_LOGGING='1',
               XDG_CONFIG_HOME=temporary, XDG_DATA_HOME=temporary,
               XDG_STATE_HOME=temporary + '/state', XDG_CACHE_HOME=temporary + '/cache',
               SEQUOIA_CONTROL_CAPTURE=str(ROOT / 'preview/control-center-native.png'))
    if args.details or args.contacts:
        env['SEQUOIA_VERIFY_DETAILS'] = ','.join(APPLETS)
        env['SEQUOIA_DETAIL_CAPTURE'] = str(ROOT / 'preview' / ('details-' + variant.lower() + ('-large' if args.large else '') + ('-rtl' if args.rtl else '')))
    if args.contacts_startup:
        env['SEQUOIA_VERIFY_CONTACTS_STARTUP'] = '1'
    if args.rtl:
        env['SEQUOIA_VERIFY_RTL'] = '1'
    if args.large:
        env['SEQUOIA_VERIFY_LARGE'] = '1'
    if args.dark:
        env['SEQUOIA_VERIFY_DARK'] = '1'
    if args.media:
        env['SEQUOIA_VERIFY_MEDIA'] = '1'
        env['SEQUOIA_CONTROL_CAPTURE'] = str(ROOT / 'preview/media-popup-native.png')
        env['QT_QUICK_BACKEND'] = 'rhi'
    env['AKONADI_INSTANCE'] = 'sequoia-preview-' + uuid.uuid4().hex
    command = ['/usr/bin/plasmawindowed', 'org.kde.plasma.systemtray']
    if args.debug:
        command = ['gdb', '-batch', '-ex', 'set environment LD_PRELOAD=' + str(directory / 'tray-qml.so') + ':' + str(helper),
                   '-ex', 'set environment SEQUOIA_CONTROL_CENTER_DIR=' + str(directory),
                   '-ex', 'run', '-ex', 'bt 25', '--args', *command]
    else:
        # Set the preload after dbus-run-session, so its daemon/services are clean.
        command = ['env', 'LD_PRELOAD=' + str(helper), 'python3', str(directory / 'launch.py'), *command]
    log = ROOT / 'preview/control-center-native.log'
    with log.open('w') as output:
        process = subprocess.Popen(command if args.reuse_bus else ['dbus-run-session', '--', *command], env=env, stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            result = process.wait(timeout=40)
        finally:
            try: os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError: pass
    print('\n'.join(line for line in log.read_text().splitlines() if any(
        token in line for token in ('PASS', 'FAIL', 'Error', 'sequoiaControlCenter', 'Dashboard was not', 'No system tray'))))
    expected = 'PASS: native media tabs, artwork and Back navigation' if args.media else 'Native audio details and return to dashboard: PASS'
    if args.details or args.contacts:
        expected = 'PASS: all detail menus and Back navigation'
    if args.contacts_startup:
        expected = 'PASS: Contacts startup remains responsive without Akonadi'
    if expected not in log.read_text():
        result = result or 1
    print('Full log:', log)
    raise SystemExit(result)
