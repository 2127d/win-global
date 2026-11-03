#!/usr/bin/env python3
"""Exercise Bluetooth activity against private mock BlueZ/OBEX buses, never real devices."""
import os
import subprocess
import sys
from pathlib import Path

if '--private-bus' not in sys.argv:
    raise SystemExit(subprocess.call(['dbus-run-session', '--', sys.executable, __file__, '--private-bus']))
os.environ.update(DBUS_SYSTEM_BUS_ADDRESS=os.environ['DBUS_SESSION_BUS_ADDRESS'], QT_QPA_PLATFORM='offscreen')
import threading
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib
from PySide6.QtCore import QTimer, QUrl, qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine

DBusGMainLoop(set_as_default=True)
class ObjectManager(dbus.service.Object):
    def __init__(self, name, objects):
        self.mock_bus = dbus.bus.BusConnection(os.environ['DBUS_SESSION_BUS_ADDRESS'])
        self.name = dbus.service.BusName(name, bus=self.mock_bus)
        self.objects = objects
        self.calls = 0
        super().__init__(self.mock_bus, '/')

    @dbus.service.method('org.freedesktop.DBus.ObjectManager', out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        self.calls += 1
        return self.objects

bluez = ObjectManager('org.bluez', {
    '/org/bluez': {'org.bluez.AgentManager1': {}, 'org.bluez.ProfileManager1': {}},
    '/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/sep1/fd0': {'org.bluez.MediaTransport1': {
        'State': 'active', 'Device': dbus.ObjectPath('/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF')}},
    '/org/bluez/hci0/dev_11_22_33_44_55_66/player0': {'org.bluez.MediaPlayer1': {'Status': 'playing'}}
})
obex = ObjectManager('org.bluez.obex', {
    '/org/bluez/obex/client/session0': {'org.bluez.obex.Session1': {'Destination': 'aa:bb:cc:dd:ee:ff'}},
    '/org/bluez/obex/client/session0/transfer0': {'org.bluez.obex.Transfer1': {
        'Status': 'active', 'Session': dbus.ObjectPath('/org/bluez/obex/client/session0')}}
})
loop = GLib.MainLoop()
threading.Thread(target=loop.run, daemon=True).start()
errors = []
def message(kind, context, text):
    if any(token in text for token in ['Error', 'Unable to assign', 'Cannot assign', 'Binding loop']):
        errors.append(text)
qInstallMessageHandler(message)
app = QGuiApplication([])
engine = QQmlEngine()
component = QQmlComponent(engine)
source = Path(__file__).resolve().parents[1] / 'desktop/control-center/bluetooth'
component.setData(('import QtQuick\nimport "' + source.as_uri() + '" as Bluetooth\n'
    'Bluetooth.BluetoothActivity { monitoring: true\n'
    'property int playingCount: Object.keys(playing).length\n'
    'property bool headphonePlaying: !!playing["/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF"]\n'
    'property bool phonePlaying: !!playing["/org/bluez/hci0/dev_11_22_33_44_55_66"]\n'
    'property int transferCount: Object.keys(transferring).length\n'
    'property bool headphoneTransfer: !!transferring["AA:BB:CC:DD:EE:FF"]\n}').encode(), QUrl())
monitor = component.create()
assert monitor, [e.toString() for e in component.errors()]

def later(delay, function):
    def check():
        try:
            function()
        except Exception as error:
            print('FAIL:', repr(error), errors, flush=True)
            app.exit(1)
    QTimer.singleShot(delay, check)

def active():
    assert monitor.property('playingCount') == 1, ('playing', monitor.property('playingCount'), bluez.calls)
    assert monitor.property('headphonePlaying') and not monitor.property('phonePlaying')
    assert monitor.property('transferCount') == 1 and monitor.property('headphoneTransfer'), ('transfer', monitor.property('transferCount'), obex.calls)
    bluez.objects['/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/sep1/fd0']['org.bluez.MediaTransport1']['State'] = 'idle'
    obex.objects['/org/bluez/obex/client/session0/transfer0']['org.bluez.obex.Transfer1']['Status'] = 'complete'
    later(1400, completed)

def completed():
    assert monitor.property('playingCount') == 0
    assert monitor.property('transferCount') == 0
    monitor.setProperty('monitoring', False)
    calls = obex.calls
    def stopped():
        assert obex.calls == calls, 'Closed popup must stop polling'
        assert not errors, errors
        print('PASS: native D-Bus decoding, active streams only, stale player exclusion, transfer completion, and polling lifecycle', flush=True)
        app.exit(0)
    later(1200, stopped)

later(1600, active)
result = app.exec()
loop.quit()
raise SystemExit(result)
