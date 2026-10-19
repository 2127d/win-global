import os,sys,tempfile,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
(root/'preview').mkdir(exist_ok=True)
if '--child' not in sys.argv:
 for mode in ('light','dark'):
  with tempfile.TemporaryDirectory(prefix='sequoia-search-border-') as temp:
   p=Path(temp)
   (p/'plasma').mkdir();(p/'plasma/desktoptheme').symlink_to(root/'themes/plasma/desktoptheme')
   (p/'icons').symlink_to(root/'themes/icons')
   (p/'plasmarc').write_text('[Theme]\nname=sequoia-eleven-'+mode+'\n')
   env=dict(os.environ,XDG_CONFIG_HOME=temp,XDG_DATA_HOME=temp,XDG_STATE_HOME=temp+'/state',XDG_CACHE_HOME=temp+'/cache',QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_QPA_PLATFORMTHEME='generic',QT_STYLE_OVERRIDE='Fusion')
   subprocess.run(['dbus-run-session','--',sys.executable,__file__,'--child',mode],env=env,check=True,timeout=20)
 sys.exit(0)
from PySide6.QtCore import QObject,Slot,QUrl,Qt
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
class Translations(QObject):
 @Slot(str,str,result=str)
 def i18nd(self,domain,text):return text
app=QApplication([])
mode=sys.argv[-1]
qml=Path(os.environ['XDG_CONFIG_HOME'])/'Search.qml'
qml.write_text('''import QtQuick
import org.kde.plasma.extras as PE
Rectangle {
 width: 268; height: 88; color: "'''+('#f2f3f5' if mode=='light' else '#252629')+'''"
 PE.SearchField { objectName: "search"; width: 220; anchors.centerIn: parent }
}''')
view=QQuickView();tr=Translations();view.rootContext().setContextObject(tr)
view.setSource(QUrl.fromLocalFile(str(qml)));assert view.status()==QQuickView.Ready,view.errors()
view.show();QTest.qWait(250)
field=view.rootObject().findChild(QObject,'search');field.forceActiveFocus(Qt.TabFocusReason)
QTest.qWait(450)
def walk(item):
 yield item
 for child in item.childItems():yield from walk(child)
frames=[item for item in walk(field) if 'FrameSvgItem' in item.metaObject().className() and item.property('imagePath') == 'widgets/lineedit']
assert len(frames)==3,[(i.metaObject().className(),i.property('prefix')) for i in frames]
base=frames[0]
for overlay in frames[1:]:
 assert overlay.x()==0 and overlay.y()==0,(overlay.property('prefix'),overlay.x(),overlay.y())
 assert overlay.size()==base.size(),(overlay.size(),base.size())
assert field.property('topPadding')==6 and field.property('bottomPadding')==6
assert view.grabWindow().save(str(root/'preview'/('search-highlight-'+mode+'.png')))
print('PASS:',mode,'native SearchField focus/hover align exactly with base; 6px text padding retained',flush=True)
view.close()
