#!/usr/bin/env python3
"""Check live rim updates and unchanged center alpha with isolated settings."""
import os
from pathlib import Path
import tempfile
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='generic',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='SequoiaElevenGlass')
with tempfile.TemporaryDirectory(prefix='sequoia-rim-check-') as directory:
    os.environ['XDG_CONFIG_HOME']=directory
    from PySide6.QtWidgets import QApplication,QMenu,QToolBar,QStyleOptionToolBar,QStyleOption,QStyle
    from PySide6.QtGui import QPalette,QColor,QImage,QPainter
    from PySide6.QtCore import QRect,QUrl,Qt
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtQuick import QQuickWindow
    from PySide6.QtTest import QTest
    (Path(directory)/'sequoiaelevenrc').write_text('[Style]\nMenuOpacity=44\n[Windeco]\nGlassRimIntensity=100\n')
    app=QApplication([])
    style=app.style()
    assert 'glass' in style.objectName().lower(),style.objectName()
    engine=QQmlApplicationEngine()
    engine.loadData(b'''import QtQuick
import org.sequoia.glass as Glass
Window { width: 360; height: 90; visible: true; color: "transparent"
    Rectangle { anchors.fill: parent; radius: 22; color: "#a8252629" }
    Glass.Rim { anchors.fill: parent; radius: 22 }
}''',QUrl('file:///rim-check.qml'))
    assert engine.rootObjects(),'Shared rim QML failed to load'
    window=engine.rootObjects()[0]
    def change(value):
        temp=Path(directory)/'replacement'
        temp.write_text(f'[Style]\nMenuOpacity=44\n[Windeco]\nGlassRimIntensity={value}\n')
        temp.replace(Path(directory)/'sequoiaelevenrc')
        QTest.qWait(180)
    def pixels(image):return bytes(image.constBits())
    def render(palette,menu=False):
        bar=QMenu() if menu else QToolBar()
        bar.setAttribute(Qt.WA_TranslucentBackground,True)
        bar.resize(360,90);bar.setPalette(palette)
        option=QStyleOption() if menu else QStyleOptionToolBar()
        option.initFrom(bar);option.rect=QRect(0,0,360,90);option.state|=QStyle.State_Active
        image=QImage(360,90,QImage.Format_ARGB32_Premultiplied);image.fill(QColor('transparent'))
        painter=QPainter(image)
        if menu:style.drawPrimitive(QStyle.PE_PanelMenu,option,painter,bar)
        else:style.drawControl(QStyle.CE_ToolBar,option,painter,bar)
        painter.end()
        bar.deleteLater()
        return image
    for dark in (False,True):
        palette=QPalette();palette.setColor(QPalette.Window,QColor(37,38,41,168) if dark else QColor(242,243,245,148));palette.setColor(QPalette.Base,palette.color(QPalette.Window));palette.setColor(QPalette.WindowText,QColor('white' if dark else 'black'))
        images=[]
        for value in (0,50,200,0):
            change(value)
            images.append((render(palette),render(palette,True),window.grabWindow()))
        for index,label in enumerate(('toolbar','menu','Quick rim')):
            seq=[row[index] for row in images]
            assert all(not frame.isNull() for frame in seq),label
            center=[frame.pixelColor(frame.width()//2,frame.height()//2).getRgb() for frame in seq]
            assert len(set(center))==1,(label,'center changed',center)
            if label=='toolbar':
                assert center[0][3]==255,center
                assert all(pixels(frame)==pixels(seq[0]) for frame in seq),'Solid toolbar acquired a glass rim'
                print('PASS', 'dark' if dark else 'light',label,'remains solid through rim changes')
                continue
            assert pixels(seq[0])!=pixels(seq[1])!=pixels(seq[2]),(label,'rim did not update')
            assert pixels(seq[0])==pixels(seq[3]),(label,'zero did not restore')
            print('PASS', 'dark' if dark else 'light',label,'live 0→50→200→0, center',center[0])
    window.close()
