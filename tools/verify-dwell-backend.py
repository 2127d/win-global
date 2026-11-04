#!/usr/bin/env python3
"""Exercise the staged dwell adapter against a private fake D-Bus service."""
import argparse,os,sys,subprocess,atexit
from pathlib import Path
from preview_session import ensure_private_session
ensure_private_session()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--main',type=Path);p.add_argument('--fake-service',action='store_true');args=p.parse_args()
service='org.kde.KWin.DwellClicker';path='/org/kde/KWin/DwellClicker'
if args.fake_service:
 import dbus,dbus.service,dbus.mainloop.glib
 from gi.repository import GLib
 dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
 connection=dbus.SessionBus();name=dbus.service.BusName(service,connection)
 class Service(dbus.service.Object):
  button=10
  writes=[]
  @dbus.service.method('org.freedesktop.DBus.Introspectable',in_signature='',out_signature='s')
  def Introspect(self):return '<node><interface name="org.kde.KWin.DwellClicker"><property name="button" type="i" access="readwrite"/></interface></node>'
  @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='s',out_signature='a{sv}')
  def GetAll(self,interface):return {'button':dbus.Int32(self.button)}
  @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='ss',out_signature='v')
  def Get(self,interface,property):return dbus.Int32(self.button)
  @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='ssv',out_signature='')
  def Set(self,interface,property,value):
   self.button=int(value);self.writes.append(self.button);self.PropertiesChanged(service,{'button':dbus.Int32(self.button)},[])
  @dbus.service.signal('org.freedesktop.DBus.Properties',signature='sa{sv}as')
  def PropertiesChanged(self,interface,changed,invalidated):pass
  @dbus.service.method('org.example.SequoiaTest',in_signature='i',out_signature='')
  def SetExternal(self,value):self.button=int(value);self.PropertiesChanged(service,{'button':dbus.Int32(self.button)},[])
  @dbus.service.method('org.example.SequoiaTest',in_signature='',out_signature='i')
  def WriteCount(self):return len(self.writes)
  @dbus.service.method('org.example.SequoiaTest',in_signature='',out_signature='i')
  def LastWrite(self):return self.writes[-1]
 obj=Service(connection,path);print('READY',flush=True);GLib.MainLoop().run();raise SystemExit()
assert args.main,'--main is required'
process=subprocess.Popen([sys.executable,__file__,'--fake-service'],stdout=subprocess.PIPE,text=True)
assert process.stdout.readline().strip()=='READY'
atexit.register(lambda: (process.terminate(),process.wait(timeout=3)))
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software')
from PySide6.QtCore import QUrl
from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlEngine,QQmlComponent,QQmlExpression
from PySide6.QtDBus import QDBusConnection,QDBusInterface
from PySide6.QtTest import QTest
app=QApplication([]);bus=QDBusConnection.sessionBus()
remote=QDBusInterface(service,path,'org.example.SequoiaTest',bus)
def call(method,*args):
 reply=remote.call(method,*args)
 assert reply.errorName()=='',reply.errorMessage()
 return reply.arguments()[0] if reply.arguments() else None
text=args.main.read_text()
watchers=text[text.index('    property int selectedButtonId:'):text.index('    Plasmoid.status:')]
controller=text[text.index('    QtObject {\n        id: nativeController'):text.index('    fullRepresentation: FullRepresentation')]
imports='''import QtQuick
import org.kde.plasma.workspace.dbus as DBus
import org.kde.config
import org.kde.kcmutils
'''
source=imports+'Item { id: root; property bool expanded: true; property bool hideOnWindowDeactivate: true; property alias controller: nativeController;\n'+watchers+controller+'\n}'
engine=QQmlEngine();component=QQmlComponent(engine);component.setData(source.encode(),QUrl())
root=component.create();assert root,component.errors()
def evaluate(expression):
 expr=QQmlExpression(engine.rootContext(),root,expression);value=expr.evaluate()[0]
 assert not expr.hasError(),expr.error().toString()
 return value
def wait_for(expression,expected):
 for _ in range(40):
  if evaluate(expression)==expected:return
  QTest.qWait(25)
 raise AssertionError((expression,evaluate(expression),expected))
wait_for('controller.available',True);wait_for('controller.selected',10)
for mode in [11,12,13,14,10]:
 evaluate(f'controller.select({mode})');wait_for('controller.selected',mode);assert call('LastWrite')==mode
 evaluate(f'controller.select({mode})');wait_for('controller.selected',0);assert call('LastWrite')==0
assert not root.property('expanded'),'Native selection should close an unpinned popup'
root.setProperty('hideOnWindowDeactivate',False);root.setProperty('expanded',True)
evaluate('controller.select(12)');wait_for('controller.selected',12);assert root.property('expanded')
call('SetExternal',14);wait_for('controller.selected',14)
count=call('WriteCount');evaluate('controller.select(99)');QTest.qWait(50);assert call('WriteCount')==count
process.terminate();process.wait(timeout=3);wait_for('controller.available',False)
evaluate('controller.select(10)');QTest.qWait(50);assert evaluate('controller.selected')==0
print('PASS: five native mode IDs, deselection, external updates, unavailable guard, and popup close/pin behaviour over private D-Bus')
