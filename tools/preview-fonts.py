#!/usr/bin/env python3
"""Render UI font comparison and verify real Qt font selection."""
import os
from pathlib import Path
import tempfile
os.environ['QT_QPA_PLATFORM'] = 'offscreen'
os.environ['QT_STYLE_OVERRIDE'] = 'Fusion'
os.environ['QT_QPA_PLATFORMTHEME'] = 'generic'
config = tempfile.TemporaryDirectory(prefix='sequoia-font-preview-')
os.environ['XDG_CONFIG_HOME'] = config.name
import importlib.util
spec = importlib.util.spec_from_file_location('font_apply', Path(__file__).with_name('apply-fonts.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
rule = Path(config.name) / 'fontconfig/conf.d/99-sequoia-ui-fonts.conf'
rule.parent.mkdir(parents=True)
rule.write_text(module.FONTCONFIG)
from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QFont, QFontInfo, QFontMetricsF, QRawFont, QImage, QPainter, QColor
from typography import FONT_FAMILY

app = QApplication([])
for weight in (QFont.Normal, QFont.Medium, QFont.DemiBold, QFont.Bold):
    for italic in (False, True):
        font = QFont(FONT_FAMILY, 10, weight)
        font.setItalic(italic)
        info = QFontInfo(font)
        assert info.family() == FONT_FAMILY and info.italic() == italic
        assert info.weight() == weight
        raw = QRawFont.fromFont(font)
        assert all(raw.supportsCharacter(ord(char)) for char in 'Settings Downloads 0123456789 — … €')

image = QImage(800, 330, QImage.Format_ARGB32_Premultiplied)
image.fill(QColor('#f2f3f5'))
painter = QPainter(image)
painter.setRenderHint(QPainter.TextAntialiasing)
menu = [('Dolphin - Downloads', 18), ('File', 170), ('Edit', 210), ('View', 248), ('Go', 294),
        ('Tools', 330), ('Settings', 380), ('Help', 446), ('Search', 490)]
for index, family in enumerate(('Nunito', FONT_FAMILY)):
    y = 24 + 153 * index
    painter.setPen(QColor('#666b75'))
    painter.setFont(QFont(FONT_FAMILY, 9))
    painter.drawText(18, y, ('Before · ' if index == 0 else 'After · ') + family + ' 10 pt')
    painter.setPen(QColor('#24262b'))
    painter.setFont(QFont(family, 10))
    for label, x in menu:
        painter.drawText(x, y + 32, label)
    painter.drawText(18, y + 65, 'Appearance  •  Documents  •  09:41  •  1,234.56  •  Settings & Privacy')
    painter.setFont(QFont(family, 10, QFont.Medium))
    painter.drawText(18, y + 98, 'Window title — clear text at the same size')
painter.end()
output = Path(__file__).resolve().parents[1] / 'preview/fonts.png'
assert image.save(str(output))
print('PASS: regular/medium/semibold/bold, upright/italic, UI glyphs; rendered', output)
