#!/usr/bin/env python3
"""Render the widget layer shader and check wallpaper-only changes."""
import os
from pathlib import Path
import tempfile
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',QSG_RENDER_LOOP='basic')
from PySide6.QtCore import QUrl,QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtQuick import QQuickView
ROOT=Path(__file__).resolve().parents[1]
app=QGuiApplication([]);view=QQuickView()
qml='''import QtQuick
import QtQuick.Effects
import "GLASS_PATH"
Rectangle {
 width: 360; height: 400; color: "#243a60"
 property bool glass: false
 Item { id: wallpaper; anchors.fill: parent
  Rectangle { anchors.fill: parent; color: "#3f728b" }
  Repeater { model: 18; Rectangle { x: index*24; width: 7; height: 400; color: "#d5d9ac" } }
 }
 ShaderEffectSource { id: crop; sourceItem: wallpaper; sourceRect: Qt.rect(60,50,240,300); visible: false }
 MultiEffect {
  id: material; x: 60; y: 50; width: 240; height: 300
  source: crop; blurEnabled: true; blur: 1; blurMax: 128; autoPaddingEnabled: false
  layer.enabled: glass
  layer.effect: GlassBackdrop {}
 }
 Rectangle { x: 115; y: 150; width: 130; height: 90; radius: 12; color: "#f2f5fa"
  Text { anchors.centerIn: parent; text: "Sharp content"; color: "#253142" }
 }
}'''.replace('GLASS_PATH',(ROOT/'desktop/glass-widget').as_uri())
with tempfile.TemporaryDirectory() as temp:
    file=Path(temp)/'check.qml';file.write_text(qml)
    view.setSource(QUrl.fromLocalFile(str(file)))
    assert view.status()==QQuickView.Ready,view.errors()
    view.show();images=[]
    def first():
        images.append(view.grabWindow());view.rootObject().setProperty('glass',True);QTimer.singleShot(700,second)
    def second():
        images.append(view.grabWindow())
        a,b=images
        changed=sum(a.pixel(x,y)!=b.pixel(x,y) for y in range(50,350) for x in range(60,300))
        assert changed>1000,changed
        assert a.copy(125,160,110,70)==b.copy(125,160,110,70),'Opaque foreground changed'
        output=os.environ.get('WIDGET_GLASS_CAPTURE')
        if output:b.save(output)
        print('Widget glass changed',changed,'background pixels; opaque foreground unchanged.',flush=True)
        app.quit()
    QTimer.singleShot(800,first);app.exec()
