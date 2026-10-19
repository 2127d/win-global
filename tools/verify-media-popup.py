#!/usr/bin/env python3
"""Test the native media popup UI with two real MPRIS services on a private bus."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from preview_session import ensure_private_session
ensure_private_session()

if '--player' in sys.argv:
    import dbus
    import dbus.service
    from dbus.mainloop.glib import DBusGMainLoop
    from gi.repository import GLib
    DBusGMainLoop(set_as_default=True)
    identity, art, log = sys.argv[-3:]
    bus = dbus.SessionBus()
    name = dbus.service.BusName('org.mpris.MediaPlayer2.Sequoia' + identity, bus)
    class Player(dbus.service.Object):
        status = 'Playing'
        position = 45000000
        shuffle = False
        loop = 'None'
        @dbus.service.method('org.freedesktop.DBus.Properties', in_signature='s', out_signature='a{sv}')
        def GetAll(self, interface):
            if interface == 'org.mpris.MediaPlayer2':
                return dict(Identity=identity, DesktopEntry='google-chrome' if identity == 'Chrome' else 'vlc', CanQuit=False,
                            CanRaise=True, HasTrackList=False, SupportedUriSchemes=dbus.Array(['file'], signature='s'),
                            SupportedMimeTypes=dbus.Array(['audio/ogg'], signature='s'))
            return dict(PlaybackStatus=self.status, LoopStatus=self.loop, Rate=1.0, Shuffle=self.shuffle, Volume=1.0,
                        Position=dbus.Int64(self.position), MinimumRate=1.0, MaximumRate=1.0, CanGoNext=True,
                        CanGoPrevious=True, CanPlay=True, CanPause=True, CanSeek=True, CanControl=True,
                        Metadata=dbus.Dictionary({'xesam:title': 'A journey beyond the ordinary' if identity == 'Chrome' else 'Midnight in motion',
                            'xesam:artist': dbus.Array(['Field Notes' if identity == 'Chrome' else 'The Quiet Hours'], signature='s'),
                            'mpris:artUrl': art, 'mpris:trackid': dbus.ObjectPath('/track/one'),
                            'mpris:length': dbus.Int64(240000000)}, signature='sv'))
        @dbus.service.method('org.freedesktop.DBus.Properties', in_signature='ss', out_signature='v')
        def Get(self, interface, key): return self.GetAll(interface)[key]
        @dbus.service.method('org.freedesktop.DBus.Properties', in_signature='ssv')
        def Set(self, interface, key, value):
            if key == 'Shuffle': self.shuffle = bool(value)
            elif key == 'LoopStatus': self.loop = str(value)
            self.record(key)
            self.PropertiesChanged(interface, {key: value}, [])
        def record(self, action):
            with open(log, 'a') as f: f.write(identity + ':' + action + '\n')
        @dbus.service.signal('org.freedesktop.DBus.Properties', signature='sa{sv}as')
        def PropertiesChanged(self, interface, changed, invalidated): pass
        @dbus.service.signal('org.mpris.MediaPlayer2.Player', signature='x')
        def Seeked(self, position): pass
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Next(self): self.record('next')
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Previous(self): self.record('previous')
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Pause(self):
            self.record('pause'); self.status = 'Paused'
            self.PropertiesChanged('org.mpris.MediaPlayer2.Player', {'PlaybackStatus': self.status}, [])
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Play(self):
            self.record('play'); self.status = 'Playing'
            self.PropertiesChanged('org.mpris.MediaPlayer2.Player', {'PlaybackStatus': self.status}, [])
        @dbus.service.method('org.mpris.MediaPlayer2.Player', in_signature='ox')
        def SetPosition(self, track, position):
            self.record('seek'); self.position = position; self.Seeked(position)
        @dbus.service.method('org.mpris.MediaPlayer2')
        def Raise(self): self.record('raise')
    player = Player(bus, '/org/mpris/MediaPlayer2')
    print('ready', flush=True)
    GLib.MainLoop().run()
    sys.exit(0)

ROOT = Path(__file__).resolve().parents[1]
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='rhi', QSG_RHI_BACKEND='opengl',
                  QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion', QSG_RENDER_LOOP='basic')
from PySide6.QtCore import QUrl, QObject, Qt, qInstallMessageHandler
from PySide6.QtGui import QPalette, QColor, QFont
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

with tempfile.TemporaryDirectory(prefix='sequoia-media-popup-') as temp:
    os.environ['XDG_CONFIG_HOME'] = temp
    dark = '--dark' in sys.argv
    large = '--large' in sys.argv
    Path(temp, 'kdeglobals').write_text((ROOT / f'themes/color-schemes/SequoiaEleven{"Dark" if dark else "Light"}Solid.colors').read_text())
    art = Path(temp, 'art.svg')
    art.write_text('''<svg xmlns="http://www.w3.org/2000/svg" width="640" height="360" viewBox="0 0 640 360"><defs><linearGradient id="g" x2="1" y2="1"><stop stop-color="#103a59"/><stop offset=".55" stop-color="#235b70"/><stop offset="1" stop-color="#e39b83"/></linearGradient></defs><rect width="640" height="360" fill="url(#g)"/><circle cx="465" cy="110" r="56" fill="#f9d5a7"/><path d="M0 340L160 140L340 330L470 200L640 355V360H0" fill="#162f45"/><path d="M40 360L250 220L430 360" fill="#29586b"/></svg>''')
    square = Path(temp, 'square.svg')
    square.write_text(art.read_text().replace('height="360" viewBox="0 0 640 360"', 'height="640" viewBox="0 0 640 360"'))
    log = Path(temp, 'actions')
    processes = []
    try:
        for identity, artwork in [('Chrome', art), ('VLC', square)]:
            p = subprocess.Popen([sys.executable, __file__, '--player', identity, artwork.as_uri(), str(log)], stdout=subprocess.PIPE, text=True)
            processes.append(p)
            assert p.stdout.readline().strip() == 'ready'
        if '--native' in sys.argv:
            sys.exit(subprocess.call([sys.executable, str(ROOT / 'tools/verify-control-center.py'), '--media', '--reuse-bus']))
        errors = []
        def message(kind, context, text):
            if any(token in text for token in ('Error', 'is not a type', 'Cannot assign', 'Unable to assign', 'Binding loop')): errors.append(text)
            print(text, file=sys.stderr)
        qInstallMessageHandler(message)
        app = QApplication([])
        palette = QPalette()
        for role, light, dark_color in [(QPalette.Window, '#f6d8dc', '#22232c'), (QPalette.WindowText, '#252332', '#f5f4f8'),
                                       (QPalette.Text, '#252332', '#f5f4f8'), (QPalette.ButtonText, '#252332', '#f5f4f8'),
                                       (QPalette.Highlight, '#3d69d8', '#88a7ff'), (QPalette.HighlightedText, '#ffffff', '#151c30')]:
            palette.setColor(role, QColor(dark_color if dark else light))
        app.setPalette(palette); app.setFont(QFont('Adwaita Sans', 13 if large else 10))
        qml = Path(temp, 'Preview.qml')
        qml.write_text(f'''import QtQuick
import org.kde.plasma.private.mpris as Mpris
import "{(ROOT / 'desktop/control-center/media').as_uri()}" as Custom
Rectangle {{
    width: 432; height: 432
    color: "{'#22232c' if dark else '#f6d8dc'}"
    Mpris.Mpris2Model {{ id: model }}
    Custom.MediaPlayerView {{ anchors.fill: parent; players: model }}
}}''')
        view = QQuickView(); view.setSource(QUrl.fromLocalFile(str(qml)))
        assert view.status() == QQuickView.Ready, view.errors()
        view.show(); QTest.qWait(1200)
        def items(item):
            result = [item]
            for child in item.childItems(): result.extend(items(child))
            return result
        def find(name):
            found = next((i for i in items(view.rootObject()) if i.objectName() == name), None)
            assert found is not None, name
            return found
        def click(name):
            button = find(name)
            assert button.property('enabled'), name
            button.forceActiveFocus(); QTest.keyClick(view, Qt.Key_Space); QTest.qWait(150)
        tabs = find('mediaPlayerTabs')
        assert tabs.property('count') == 3, 'Expected Auto, Chrome and VLC tabs'
        click('mediaPlayerTab:Chrome')
        assert find('mediaTrackTitle').property('text') == 'A journey beyond the ordinary'
        QTest.qWait(200)
        suffix = ('-dark' if dark else '-light') + ('-large' if large else '')
        assert view.grabWindow().save(str(ROOT / ('preview/media-popup' + suffix + '.png')))
        click('mediaPlayPause'); assert not find('sequoiaMediaPlayer').property('playing')
        click('mediaPlayPause'); assert find('sequoiaMediaPlayer').property('playing')
        click('mediaPrevious'); click('mediaNext'); click('mediaShuffle'); click('mediaRepeat')
        seek = find('mediaSeekSlider'); seek.forceActiveFocus(); QTest.keyClick(view, Qt.Key_Right); QTest.qWait(200)
        click('mediaPlayerTab:VLC')
        assert find('mediaTrackTitle').property('text') == 'Midnight in motion'
        click('mediaPlayPause')
        assert not find('sequoiaMediaPlayer').property('playing')
        actions = log.read_text().splitlines()
        for action in ('Chrome:pause', 'Chrome:play', 'Chrome:previous', 'Chrome:next', 'Chrome:Shuffle', 'Chrome:LoopStatus', 'Chrome:seek', 'VLC:pause'):
            assert action in actions, (action, actions)
        # A disappeared selected player must recover to another available source.
        processes[1].terminate(); processes[1].wait(timeout=3); QTest.qWait(300)
        assert tabs.property('count') == 2
        assert find('mediaTrackTitle').property('text') == 'A journey beyond the ordinary'
        processes[0].terminate(); processes[0].wait(timeout=3); QTest.qWait(300)
        assert find('mediaTrackTitle').property('text') == 'Nothing playing yet'
        assert not find('mediaPlayPause').property('enabled')
        assert not errors, errors
        print('PASS: player tabs, artwork, playback, seek, shuffle/repeat, player removal and empty state')
    finally:
        for p in processes:
            if p.poll() is None: p.terminate(); p.wait(timeout=3)
