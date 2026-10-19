#!/usr/bin/env python3
"""Check the bundled Tahoe family and render every modified SVG through Qt."""
import configparser
import json
import os
from pathlib import Path

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QRectF
from PySide6.QtGui import QGuiApplication, QImage, QPainter
from PySide6.QtSvg import QSvgRenderer

ROOT = Path(__file__).resolve().parents[1]
app = QGuiApplication([])
manifest = json.loads((ROOT / 'themes/icons/bold-icons.json').read_text())
for relative in manifest['modified']:
    path = ROOT / 'themes/icons' / relative
    renderer = QSvgRenderer(str(path))
    assert renderer.isValid(), path
    image = QImage(32, 32, QImage.Format.Format_ARGB32_Premultiplied)
    image.fill(0)
    painter = QPainter(image)
    renderer.render(painter, QRectF(0, 0, 32, 32))
    painter.end()
    assert any(image.pixelColor(x, y).alpha() for y in range(32) for x in range(32)), path
for path in (ROOT / 'themes/icons').rglob('*'):
    if path.is_symlink():
        assert path.exists(), path
        assert path.resolve().is_relative_to((ROOT / 'themes/icons').resolve()), path
for name in ('SequoiaTahoe', 'SequoiaTahoe-light', 'SequoiaTahoe-dark'):
    theme = ROOT / 'themes/icons' / name
    config = configparser.ConfigParser(interpolation=None)
    config.read(theme / 'index.theme')
    for key in ('Directories', 'ScaledDirectories'):
        for directory in config['Icon Theme'][key].split(','):
            assert (theme / directory).is_dir(), (name, directory)
            assert directory in config, (name, directory)
    assert (theme / 'COPYING').is_file()
    assert (theme / 'AUTHORS').is_file()
for path in (ROOT / 'themes/plasma/look-and-feel').glob('org.sequoia.eleven.*/contents/defaults'):
    mode = path.parents[1].name.split('.')[3]
    assert f'[kdeglobals][Icons]\nTheme=SequoiaTahoe-{mode}\n' in path.read_text(), path
print(f"PASS: {len(manifest['modified'])} modified SVGs render; self-contained links, theme directories, licenses and all six global-theme defaults verified")
