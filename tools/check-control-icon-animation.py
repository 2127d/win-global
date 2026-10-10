import os
os.environ.update(QT_QPA_PLATFORM='wayland',QT_QPA_PLATFORMTHEME='generic',QT_STYLE_OVERRIDE='Fusion',QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',QSG_RENDER_LOOP='basic')
from pathlib import Path
from PySide6.QtCore import QUrl,QObject
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
root=Path(__file__).resolve().parents[1]
(root/'.staging').mkdir(exist_ok=True)
(root/'preview').mkdir(exist_ok=True)
qml=root/'.staging/control-icon-animation.qml'
qml.write_text('''import QtQuick
import org.kde.kirigami as Kirigami
import "../plasma/control-center" as Custom
Rectangle {
    width: 144; height: 144; color: "#303238"
    property int duration: Kirigami.Units.longDuration
    Custom.ControlCenterIcon {
        objectName: "testIcon"
        anchors.centerIn: parent
        width: 96; height: 96; color: "#f2f3f5"
    }
}''')
app=QApplication([]); view=QQuickView();view.setSource(QUrl.fromLocalFile(str(qml)))
assert view.status()==QQuickView.Ready,view.errors()
view.show();QTest.qWait(100)
icon=view.rootObject().findChild(QObject,'testIcon')
duration=view.rootObject().property('duration')
def capture(name):
 assert view.grabWindow().save(str(root/'preview'/('control-icon-'+name+'.png')))
capture('idle')
icon.setProperty('hovered',True)
QTest.qWait(duration+100)
assert abs(icon.property('switchPosition')-.16)<.001
capture('hover')
icon.setProperty('expanded',True)
if duration>80:
 QTest.qWait(duration//2)
 assert .16<icon.property('switchPosition')<1
QTest.qWait(duration+100)
assert icon.property('switchPosition')==1
capture('open')
icon.setProperty('hovered',False);icon.setProperty('expanded',False)
QTest.qWait(duration+100)
assert icon.property('switchPosition')==0
print('PASS: hover, animated opening, closing, and captures; duration',duration)
