#!/usr/bin/env python3
"""Check that a D-Bus activated Activity Manager uses only temporary state."""
import os
from pathlib import Path
import subprocess
import sys

if '--child' not in sys.argv:
    state = Path(os.environ.get('XDG_STATE_HOME', Path.home() / '.local/state')) / 'kactivitymanagerdstaterc'
    before = state.read_bytes() if state.exists() else None
    subprocess.run([sys.executable, __file__, '--child'], check=True, timeout=30)
    assert (state.read_bytes() if state.exists() else None) == before, 'Live activity state changed'
    print('PASS: private service environment and unchanged live Activity state')
else:
    from preview_session import ensure_private_session
    ensure_private_session()
    import dbus
    bus = dbus.SessionBus()
    activity = dbus.Interface(bus.get_object('org.kde.ActivityManager', '/ActivityManager/Activities'),
                              'org.kde.ActivityManager.Activities')
    activity.ListActivities()
    daemon = dbus.Interface(bus.get_object('org.freedesktop.DBus', '/org/freedesktop/DBus'),
                            'org.freedesktop.DBus')
    pid = daemon.GetConnectionUnixProcessID('org.kde.ActivityManager')
    environ = dict(entry.split(b'=', 1) for entry in Path(f'/proc/{pid}/environ').read_bytes().split(b'\0') if b'=' in entry)
    for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE'):
        key = f'XDG_{kind}_HOME'
        assert environ[key.encode()].decode() == os.environ[key], key
        assert Path(os.environ[key]).is_relative_to(os.environ['SEQUOIA_PRIVATE_PREVIEW'])
