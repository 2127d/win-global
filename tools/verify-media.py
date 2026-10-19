#!/usr/bin/env python3
"""Exercise real MPRIS discovery and playback actions on an isolated D-Bus."""
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
    bus=dbus.SessionBus()
    name=dbus.service.BusName('org.mpris.MediaPlayer2.SequoiaTest',bus)
    class Player(dbus.service.Object):
        status='Playing'
        @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='s',out_signature='a{sv}')
        def GetAll(self,interface):
            with open(sys.argv[-1]+'.fetch','a') as output:output.write(interface+'\n')
            if interface == 'org.mpris.MediaPlayer2':
                return dict(Identity='Sequoia Test',CanQuit=False,CanRaise=False,HasTrackList=False,SupportedUriSchemes=dbus.Array(['file'],signature='s'),SupportedMimeTypes=dbus.Array(['audio/ogg'],signature='s'))
            return dict(PlaybackStatus=self.status,LoopStatus='None',Rate=1.0,Shuffle=False,Volume=1.0,
                        Position=dbus.Int64(0),MinimumRate=1.0,MaximumRate=1.0,
                        CanGoNext=True,CanGoPrevious=True,CanPlay=True,CanPause=True,CanSeek=False,CanControl=True,
                        Metadata=dbus.Dictionary({'xesam:title':'Test track','xesam:artist':dbus.Array(['Test artist'],signature='s'),
                                                 'mpris:trackid':dbus.ObjectPath('/track/one'),'mpris:length':dbus.Int64(180000000)},signature='sv'))
        @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='ss',out_signature='v')
        def Get(self,interface,key):return self.GetAll(interface)[key]
        def record(self,action):
            with open(sys.argv[-1],'a') as output:output.write(action+'\n')
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Next(self):self.record('next')
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Previous(self):self.record('previous')
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def PlayPause(self):self.record('playPause')
        @dbus.service.signal('org.freedesktop.DBus.Properties',signature='sa{sv}as')
        def PropertiesChanged(self,interface,changed,invalidated):pass
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Pause(self):
            self.record('pause');self.status='Paused'
            self.PropertiesChanged('org.mpris.MediaPlayer2.Player',{'PlaybackStatus':self.status},[])
        @dbus.service.method('org.mpris.MediaPlayer2.Player')
        def Play(self):
            self.record('play');self.status='Playing'
            self.PropertiesChanged('org.mpris.MediaPlayer2.Player',{'PlaybackStatus':self.status},[])
    player=Player(bus,'/org/mpris/MediaPlayer2')
    print('ready',flush=True)
    GLib.MainLoop().run()
    sys.exit(0)

os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software')
from PySide6.QtCore import QUrl,QTimer,QObject,QMetaObject,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sequoia-media-') as temp:
    log=Path(temp)/'actions'
    player=subprocess.Popen([sys.executable,__file__,'--player',str(log)],stdout=subprocess.PIPE,text=True)
    try:
        assert player.stdout.readline().strip() == 'ready'
        app=QGuiApplication([])
        qInstallMessageHandler(lambda kind,context,message: print(message,flush=True))
        qml=Path(temp)/'Test.qml'
        qml.write_text(f'''import QtQuick
import "{(root/'themes/lockscreen').as_uri()}" as Lock
Item {{
    width: 392; height: 156
    Timer {{ id: fadeoutTimer; interval: 10000 }}
    Lock.MediaControls {{ anchors.fill: parent }}
}}
''')
        view=QQuickView();view.setSource(QUrl.fromLocalFile(str(qml)))
        assert view.status() == QQuickView.Ready, view.errors()
        view.show()
        qInstallMessageHandler(lambda kind,context,message: print(message,flush=True))
        failures=[]
        def check():
            def find_card(item):
                if item.objectName() == 'sequoiaMediaCard':return item
                for child in item.childItems():
                    found=find_card(child)
                    if found:return found
                return None
            card=find_card(view.rootObject())
            if not card or card.property('trackTitle') != 'Test track' or not card.property('playing'):
                print([(x.metaObject().className(),x.objectName()) for x in view.rootObject().findChildren(QObject)],flush=True)
                print('Repeater counts:',[x.property('count') for x in view.rootObject().findChildren(QObject) if x.metaObject().className() == 'QQuickRepeater'],flush=True)
                print('Card:',card, 'title:',card.property('trackTitle') if card else None, flush=True)
                print('Fetches:',Path(str(log)+'.fetch').read_text() if Path(str(log)+'.fetch').exists() else 'none',flush=True)
                failures.append('MPRIS model did not reach the media card');app.quit();return
            for action in ['previous','playPause','next']:
                if not QMetaObject.invokeMethod(card,action):failures.append(action)
            def resume():
                if card.property('playing'):failures.append('Pause state did not update')
                QMetaObject.invokeMethod(card,'playPause')
                def finish():
                    if not card.property('playing'):failures.append('Play state did not update')
                    app.quit()
                QTimer.singleShot(250,finish)
            QTimer.singleShot(250,resume)
        QTimer.singleShot(1800,check)
        app.exec()
        assert not failures, failures
        assert log.read_text().splitlines() == ['previous','pause','next','play']
        print('MPRIS discovery, metadata, previous, play/pause, and next passed on an isolated bus.')
    finally:
        player.terminate();player.wait(timeout=3)
