#!/usr/bin/env python3
"""Render a native-size and enlarged before/after sheet using Qt's SVG renderer."""
import os
from pathlib import Path
import re

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QByteArray, QRectF
from PySide6.QtGui import QColor, QFont, QGuiApplication, QImage, QPainter
from PySide6.QtSvg import QSvgRenderer

ROOT = Path(__file__).resolve().parents[1]
app = QGuiApplication([])
samples = [
    ('Expand', 'actions/22/go-down.svg'),
    ('Notifications', 'status/22/notifications.svg'),
    ('Brightness', 'status/22/brightness-high.svg'),
    ('Bluetooth', 'devices/22/bluetooth.svg'),
    ('Volume', 'status/22/audio-volume-high.svg'),
    ('Updates', 'status/22/update-low.svg'),
    ('Copy', 'actions/22/edit-copy.svg'),
    ('Search', 'actions/22/edit-find.svg'),
    ('Settings', 'actions/22/configure.svg'),
    ('Back', 'actions/22/go-previous.svg'),
    ('Refresh', 'actions/22/view-refresh.svg'),
    ('Close', 'actions/22/window-close.svg'),
]
image = QImage(1220, 720, QImage.Format.Format_ARGB32_Premultiplied)
image.fill(QColor('#f2f3f5'))
painter = QPainter(image)
painter.setRenderHint(QPainter.RenderHint.Antialiasing)
for row, mode in enumerate(('light', 'dark')):
    y = row * 360
    background, foreground = ('#f2f3f5', '#26282c') if mode == 'light' else ('#252629', '#f2f3f5')
    painter.fillRect(0, y, 1220, 360, QColor(background))
    painter.setPen(QColor(foreground))
    painter.setFont(QFont('Sans Serif', 14))
    painter.drawText(24, y + 32, f'Sequoia Tahoe — {mode.title()}')
    painter.setFont(QFont('Sans Serif', 10))
    painter.drawText(24, y + 58, 'Original and heavier contours at 3×, with native 22px icons below each pair')
    for col, (label, relative) in enumerate(samples):
        x = 16 + col * 100
        painter.setFont(QFont('Sans Serif', 8))
        painter.drawText(QRectF(x, y + 75, 100, 30), 0x84, label)
        for variant, base in enumerate((Path.home() / f'.local/share/icons/MacTahoe-blue-{mode}',
                                        ROOT / f'themes/icons/SequoiaTahoe-{mode}')):
            path = base / relative
            if not path.exists():
                raise RuntimeError(path)
            # Match KDE's semantic text color so the sheet compares contours,
            # not the upstream files' inconsistent standalone default colors.
            text = path.read_text()
            text = re.sub(r'(\.ColorScheme-Text\s*\{\s*color:)\s*[^;]+', r'\g<1>' + foreground, text)
            renderer = QSvgRenderer(QByteArray(text.encode()))
            assert renderer.isValid(), path
            py = y + 110 + variant * 115
            renderer.render(painter, QRectF(x + 17, py, 66, 66))
            renderer.render(painter, QRectF(x + 39, py + 72, 22, 22))
    painter.drawText(24, y + 355, 'Top: original     Bottom: Sequoia Tahoe')
painter.end()
output = ROOT / 'preview/tahoe-icons.png'
assert image.save(str(output))
print(output)
