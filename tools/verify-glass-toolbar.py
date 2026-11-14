#!/usr/bin/env python3
"""Verify toolbars stay opaque in light/dark Glass and regular modes."""
import os
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='generic')
from PySide6.QtWidgets import QApplication,QStyleFactory,QToolBar,QStyleOptionToolBar,QStyle
from PySide6.QtGui import QPalette,QColor,QImage,QPainter
from PySide6.QtCore import QRect
app=QApplication([])
for dark in (False,True):
    expected=QColor(37,38,41,168) if dark else QColor(242,243,245,148)
    for name in ('SequoiaEleven','SequoiaElevenGlass'):
        style=QStyleFactory.create(name);assert style,name
        bar=QToolBar();bar.setStyle(style);bar.resize(360,56)
        palette=QPalette();palette.setColor(QPalette.Window,expected);palette.setColor(QPalette.WindowText,QColor('white' if dark else 'black'))
        bar.setPalette(palette)
        option=QStyleOptionToolBar();option.initFrom(bar);option.rect=QRect(0,0,360,56)
        image=QImage(360,56,QImage.Format_ARGB32_Premultiplied);image.fill(QColor('transparent'))
        painter=QPainter(image);style.drawControl(QStyle.CE_ToolBar,option,painter,bar);painter.end()
        pixel=image.pixelColor(180,28)
        print(name,'dark' if dark else 'light',pixel.getRgb(),flush=True)
        assert pixel.alpha()==255
        assert palette.color(QPalette.Window).alpha()==expected.alpha(), 'Canvas palette changed'
        bar.deleteLater()
print('Solid toolbar surfaces verified in both modes/styles; canvas alpha preserved.')
