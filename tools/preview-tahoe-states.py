#!/usr/bin/env python3
"""Render actual rounded device states in both toolbar palettes."""
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
from PySide6.QtCore import QByteArray, QRectF
from PySide6.QtGui import QColor,QFont,QGuiApplication,QImage,QPainter
from PySide6.QtSvg import QSvgRenderer
from tahoe_material import ROOT,svg

app=QGuiApplication([])
image=QImage(1280,860,QImage.Format.Format_ARGB32_Premultiplied);image.fill(QColor('#f4f5f7'))
p=QPainter(image);p.setRenderHint(QPainter.RenderHint.Antialiasing)
def text(x,y,label,size=12,color='#263244',bold=False):
    font=QFont('Sans Serif',size);font.setBold(bold);p.setFont(font);p.setPen(QColor(color));p.drawText(x,y,label)
text(30,44,'Sequoia Tahoe · Device states',24,bold=True)
text(30,75,'Charge, signal strength and microphone sensitivity remain distinct · native 22 px and 32 px',12)
rows=[('Battery',[f'sequoia_battery_{n}_normal' for n in range(0,101,10)]),
      ('Charging',[f'sequoia_battery_{n}_charging' for n in range(0,101,10)]),
      ('Wi-Fi',[f'sequoia_wifi_{n}_normal' for n in (0,20,40,60,80,100)]),
      ('Secured',[f'sequoia_wifi_{n}_locked' for n in (0,20,40,60,80,100)]),
      ('Limited',[f'sequoia_wifi_{n}_limited' for n in (0,20,40,60,80,100)]),
      ('Microphone',[f'sequoia_mic_{s}' for s in ('low','medium','high')])]
for panel,(background,color) in enumerate([('#ffffff','#363636'),('#222b36','#f2f2f2')]):
    top=100+panel*370
    p.setPen(QColor('#dce1e8' if panel==0 else background));p.setBrush(QColor(background));p.drawRoundedRect(QRectF(30,top,1220,350),20,20)
    for row,(label,symbols) in enumerate(rows):
        y=top+28+row*53
        text(50,y+17,label,11,color,True)
        for col,symbol in enumerate(symbols):
            x=185+col*94
            for size,offset in ((22,0),(32,30)):
                renderer=QSvgRenderer(QByteArray(svg(symbol,size,color).encode()));assert renderer.isValid()
                renderer.render(p,QRectF(x+offset,y+(32-size)/2,size,size))
p.end()
output=ROOT/'preview/tahoe-material-states.png';image.save(str(output));print(output)
