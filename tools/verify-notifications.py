#!/usr/bin/env python3
"""Exercise native notification delivery and interaction on a private D-Bus."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile
from typography import font

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dark', action='store_true')
parser.add_argument('--large', action='store_true')
parser.add_argument('--rtl', action='store_true')
args = parser.parse_args()
helper = ROOT / '.staging/verify-notifications.so'
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6DBus', 'Qt6Test'], text=True))
subprocess.run(['c++', '-std=c++17', '-shared', '-fPIC', str(ROOT/'tools/verify-notifications.cpp'), '-o', str(helper), *flags, '-lnotificationmanager'], check=True)
variant = 'Dark' if args.dark else 'Light'
suffix = variant.lower() + ('-large' if args.large else '') + ('-rtl' if args.rtl else '')
with tempfile.TemporaryDirectory(prefix='sequoia-notification-') as temporary:
    path = Path(temporary)
    (path/'kdeglobals').write_text((ROOT/f'themes/color-schemes/SequoiaEleven{variant}Solid.colors').read_text()
        + f'\n[General]\nfont={font(13 if args.large else 10)}\nsmallestReadableFont={font(11 if args.large else 9)}\n'
        + f'\n[Icons]\nTheme=SequoiaTahoe-{variant.lower()}\n')
    (path/'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant.lower()}\n')
    (path/'plasma').symlink_to(ROOT/'themes/plasma')
    (path/'icons').symlink_to(ROOT/'themes/icons')
    env = dict(os.environ, XDG_CONFIG_HOME=temporary, XDG_DATA_HOME=temporary,
        XDG_CACHE_HOME=temporary+'/cache', XDG_STATE_HOME=temporary+'/state',
        QT_QUICK_BACKEND='software', QT_FORCE_STDERR_LOGGING='1',
        SEQUOIA_NOTIFICATION_CAPTURE=str(ROOT/'preview'/('notifications-'+suffix)))
    if args.rtl: env['SEQUOIA_NOTIFICATION_RTL'] = '1'
    command = ['dbus-run-session', '--', 'env', 'LD_PRELOAD='+str(helper), 'python3',
        str(ROOT/'.staging/control-center-data/sequoia-eleven/control-center/launch.py'),
        'plasmawindowed', 'org.kde.plasma.notifications']
    logfile = ROOT/'preview'/('notifications-'+suffix+'.log')
    with logfile.open('w') as output:
        process = subprocess.Popen(command, env=env, stdout=output, stderr=output, start_new_session=True)
        try: code = process.wait(timeout=30)
        finally:
            try: os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError: pass
    log = logfile.read_text()
    print(log[-5500:])
    raise SystemExit(code or (0 if 'PASS: native notification delivery' in log else 1))
