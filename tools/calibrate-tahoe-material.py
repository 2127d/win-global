#!/usr/bin/env python3
"""Choose minimal optical insets using Qt's native SVG rasterizer."""
import json
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
from PySide6.QtCore import QByteArray
from PySide6.QtGui import QGuiApplication,QImage,QPainter
from PySide6.QtSvg import QSvgRenderer
import tahoe_material as material

app=QGuiApplication([])
material.register_states(material.GROUPS, material.SOURCE.parent)
scales={}
for symbol in material.symbols():
    for size in (16,22,24,32):
        for percent in range(100,74,-1):
            scale=percent/100
            material.OPTICAL_SCALES.setdefault(symbol,{})[str(size)]=scale
            material.svg.cache_clear()
            renderer=QSvgRenderer(QByteArray(material.svg(symbol,size,'#363636').encode()))
            if not renderer.isValid(): raise RuntimeError(symbol)
            image=QImage(size,size,QImage.Format.Format_ARGB32_Premultiplied);image.fill(0)
            painter=QPainter(image);renderer.render(painter);painter.end()
            edge=[(n,0) for n in range(size)]+[(n,size-1) for n in range(size)]+[(0,n) for n in range(size)]+[(size-1,n) for n in range(size)]
            if not any(image.pixelColor(x,y).alpha() for x,y in edge):
                scales.setdefault(symbol,{})[str(size)]=scale
                break
        else: raise RuntimeError(f'Cannot fit {symbol} at {size}px')
(material.SOURCE/'optical-sizes.json').write_text(json.dumps(scales,indent=2)+'\n')
print(f'Calibrated {len(scales)} glyphs and state variants at 16/22/24/32 pixels')
