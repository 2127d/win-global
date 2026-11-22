#!/usr/bin/env python3
"""Exercise per-section tooltip geometry and live reload without changing user settings."""
import os,sys,tempfile,subprocess,shutil,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if '--child' not in sys.argv:
    with tempfile.TemporaryDirectory(prefix='sequoia-tooltip-policy-') as temp:
        base=Path(temp);qml=base/'data/sequoia-eleven/newstuff/qml';qml.mkdir(parents=True)
        for name in ('config','cache','state'): (base/name).mkdir()
        from settings_popup_style import build
        build(qml/'org/kde/desktop')
        glass=qml/'org/sequoia/glass';glass.mkdir(parents=True)
        shutil.copy2(ROOT/'.staging/glass-material/libsequoiaglassplugin.so',glass)
        (glass/'qmldir').write_text('module org.sequoia.glass\noptional plugin sequoiaglassplugin\n')
        core=qml/'org/kde/plasma/core';core.mkdir(parents=True)
        shutil.copy2(ROOT/'themes/application-style/quick/plasma/DialogBackground.qml',core)
        fingerprint=Path('/usr/lib64/qt6/qml/org/kde/desktop/ToolTip.qml')
        (qml/'manifest.json').write_text(json.dumps({'sources':{str(fingerprint):hashlib.sha256(fingerprint.read_bytes()).hexdigest()}}))
        env=dict(os.environ,QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='SequoiaEleven',QT_QPA_PLATFORMTHEME='generic',QT_PLUGIN_PATH=str(ROOT/'.staging/desktop/usr/lib64/qt6/plugins'),QML_IMPORT_PATH=str(qml),QML2_IMPORT_PATH=str(qml),QT_QUICK_CONTROLS_STYLE='org.kde.desktop')
        for key in ('DATA','CONFIG','CACHE','STATE'):env['XDG_'+key+'_HOME']=str(base/key.lower())
        subprocess.run([sys.executable,__file__,'--child'],env=env,check=True)
    raise SystemExit()
from PySide6.QtWidgets import QApplication,QStyle,QStyleOption,QWidget
from PySide6.QtCore import QUrl,QObject,QMetaObject,QPoint,QRect,QSettings,Qt
from PySide6.QtQuick import QQuickView
from PySide6.QtQml import QQmlComponent
from PySide6.QtGui import QImage,QPainter
from PySide6.QtTest import QTest
app=QApplication([])
view=QQuickView();component=QQmlComponent(view.engine())
component.setData(b'''import QtQuick
import QtQuick.Controls as C
import org.kde.plasma.core as PC
Item { width:500; height:300; readonly property int topLocation:PC.Types.TopEdge; readonly property int bottomLocation:PC.Types.BottomEdge
 PC.Dialog { mainItem: Item { width:100; height:70 } }
 PC.ToolTipArea { objectName:"area"; width:100; height:40; active:true; location: PC.Types.BottomEdge; mainText:"Firefox" }
 C.ToolTip { objectName:"quickTip"; text:"A longer tooltip for checking rounded rectangle corners"; visible:true }
}''',QUrl())
root=component.create();assert root,component.errors();view.setContent(QUrl(),component,root);view.show();QTest.qWait(200)
area=root.findChild(QObject,'area');quick=root.findChild(QObject,'quickTip')
policy=QSettings(str(Path(os.environ['XDG_CONFIG_HOME'])/'sequoiatooltipsrc'),QSettings.IniFormat)
def set_shape(section,shape):policy.setValue('Tooltips/'+section,shape);policy.sync();QTest.qWait(180)
for shape in ('pill','rounded','square'):
    set_shape('Applications',shape)
    bg=quick.property('background');w=bg.property('width');h=bg.property('height')
    expected=min(w,h)/2 if shape=='pill' else min(20,min(w,h)/2) if shape=='rounded' else 0
    assert bg.property('radius')==expected,(shape,bg.property('radius'),expected)
    option=QStyleOption();widget=QWidget();option.initFrom(widget);option.rect=QRect(0,0,260,100)
    image=QImage(260,100,QImage.Format_ARGB32_Premultiplied);image.fill(Qt.transparent)
    painter=QPainter(image);app.style().drawPrimitive(QStyle.PE_PanelTipLabel,option,painter,widget);painter.end()
    assert (image.pixelColor(3,3).alpha()>200)==(shape=='square')
    assert (image.pixelColor(10,10).alpha()>200)==(shape!='pill')
    print('PASS: native and Quick application shape',shape)
previewComponent=QQmlComponent(view.engine());previewComponent.setData(b'import QtQuick; Item { property bool isWin:true; implicitWidth:260; implicitHeight:150 }',QUrl());preview=previewComponent.create();area.setProperty('mainItem',preview)
QMetaObject.invokeMethod(area,'showToolTip');QTest.qWait(250)
tip=next(w for w in app.allWindows() if w.metaObject().className()=='ToolTipDialog' and w.isVisible())
for shape in ('pill','rounded','square'):
    set_shape('RunningApps',shape)
    assert tip.property('margin')==12
    assert tip.mask().contains(QPoint(3,3))==(shape=='square')
    assert tip.mask().contains(QPoint(10,10))==(shape!='pill')
    assert getattr(tip.property('borders'),'value',tip.property('borders'))==0
    print('PASS: running-app shape, matching mask, shadow removal and panel gap',shape)
for location,direction in [(root.property('topLocation'),Qt.BottomEdge),(root.property('bottomLocation'),Qt.TopEdge)]:
    # Native panel anchor contract; offscreen Qt does not reposition windows.
    QMetaObject.invokeMethod(area,'hideToolTip');area.setProperty('location',location)
    QMetaObject.invokeMethod(area,'showToolTip');QTest.qWait(150)
    assert tip.property('margin')==12
    assert tip.property('popupDirection')==direction
    assert not tip.property('floating')
print('PASS: top/bottom panel anchoring with 12px margin')
set_shape('Desktop','pill');preview.setProperty('isWin',False);QTest.qWait(150)
assert not tip.mask().contains(QPoint(10,10));assert tip.property('margin')==0
set_shape('Desktop','square');assert tip.mask().contains(QPoint(3,3))
assert policy.value('Tooltips/RunningApps')=='square'
print('PASS: independent desktop shape and safe regular/preview switching')
QMetaObject.invokeMethod(area,'hideToolTip');view.close()
