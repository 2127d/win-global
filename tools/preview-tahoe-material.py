#!/usr/bin/env python3
"""Render the actual icon SVGs with Qt at enlarged and native UI sizes."""
import argparse
import os
from pathlib import Path
import re
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QByteArray, QRectF
from PySide6.QtGui import QColor, QFont, QGuiApplication, QImage, QPainter
from PySide6.QtSvg import QSvgRenderer

ROOT = Path(__file__).resolve().parents[1]
SAMPLES = [
    ('Search', 'actions/22/edit-find.svg'), ('Settings', 'actions/22/configure.svg'),
    ('Copy', 'actions/22/edit-copy.svg'), ('Paste', 'actions/22/edit-paste.svg'),
    ('Refresh', 'actions/22/view-refresh.svg'), ('Delete', 'actions/22/edit-delete.svg'),
    ('Back', 'actions/22/go-previous.svg'), ('Expand', 'actions/22/go-down.svg'),
    ('Close', 'actions/22/window-close.svg'), ('Save', 'actions/22/document-save.svg'),
    ('Play', 'actions/22/media-playback-start.svg'), ('Share', 'actions/22/document-share.svg'),
    ('Notifications', 'status/22/notifications.svg'), ('Volume', 'status/22/audio-volume-high.svg'),
    ('Muted', 'status/22/audio-volume-muted.svg'), ('Brightness', 'status/22/brightness-high.svg'),
    ('Bluetooth', 'devices/22/bluetooth.svg'), ('Wi-Fi', 'status/22/network-wireless-100.svg'),
]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--capture-before', action='store_true')
args = parser.parse_args()
baseline = ROOT / '.staging/tahoe-material-before'
theme = ROOT / 'themes/icons/SequoiaTahoe-light'
if args.capture_before:
    for _, path in SAMPLES:
        target = baseline / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((theme / path).read_bytes())
    print('Saved current Tahoe comparison icons')
    raise SystemExit(0)

app = QGuiApplication([])
image = QImage(1280, 854, QImage.Format.Format_ARGB32_Premultiplied)
image.fill(QColor('#f4f5f7'))
p = QPainter(image); p.setRenderHint(QPainter.RenderHint.Antialiasing)
def label(x, y, text, size=11, color='#667080', bold=False):
    font=QFont('Sans Serif',size); font.setBold(bold);p.setFont(font);p.setPen(QColor(color));p.drawText(x,y,text)
def icon(path,x,y,size,color):
    data=path.read_text()
    data=re.sub(r'(\.ColorScheme-Text\s*\{\s*color:)\s*[^;]+',r'\g<1>'+color,data)
    renderer=QSvgRenderer(QByteArray(data.encode()));assert renderer.isValid(),path
    renderer.render(p,QRectF(x,y,size,size))
label(30,43,'Sequoia Tahoe',24,'#202938',True)
label(30,74,'Rounded actions / Material 3 trial',14,'#4d6079')
label(30,101,'Current → Rounded   ·   Real SVG renders at 44 px, with native 16 / 22 / 24 px samples',10)
for index,(name,path) in enumerate(SAMPLES):
    x=30+(index%6)*207; y=122+(index//6)*170
    p.setPen(QColor('#e0e4ea'));p.setBrush(QColor('#ffffff'));p.drawRoundedRect(QRectF(x,y,194,156),16,16)
    label(x+14,y+25,name,11,'#273244',True)
    icon(baseline/path,x+25,y+43,44,'#303a49')
    label(x+88,y+70,'→',13,'#99a3b2')
    icon(theme/path,x+124,y+43,44,'#303a49')
    label(x+19,y+114,'CURRENT',8)
    for size,dx in [(16,114),(22,137),(24,165)]:
        icon(theme/path,x+dx-8,y+102+(24-size)/2,size,'#303a49')
    label(x+111,y+144,'16 / 22 / 24',8)
p.setPen(QColor('#dce2e9'));p.setBrush(QColor('#ffffff'));p.drawRoundedRect(QRectF(30,652,1229,77),17,17)
label(50,679,'Light toolbar',10,'#536074',True)
for i,(_,path) in enumerate(SAMPLES[:12]): icon(theme/path,250+i*70,677,24,'#303a49')
p.setPen(QColor('#303c4e'));p.setBrush(QColor('#202936'));p.drawRoundedRect(QRectF(30,745,1229,77),17,17)
label(50,773,'Dark toolbar + tray',10,'#c0cada',True)
for i,(_,path) in enumerate(SAMPLES[6:]): icon(theme/path,250+i*70,770,24,'#eef2f8')
p.end()
output=ROOT/'preview/tahoe-material-rounded.png';output.parent.mkdir(exist_ok=True)
assert image.save(str(output));print(output)
