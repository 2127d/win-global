#!/usr/bin/env python3
"""Check the bundled Tahoe family and render every modified SVG through Qt."""
import configparser
import hashlib
import json
import os
from pathlib import Path

os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
from PySide6.QtCore import QRectF, QSize
from PySide6.QtGui import QGuiApplication, QIcon, QImage, QPainter
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

material = json.loads((ROOT / 'themes/icons/material-icons.json').read_text())
rendered = set()
for relative, info in material['generated'].items():
    path = ROOT / 'themes/icons' / relative
    assert hashlib.sha256(path.read_bytes()).hexdigest() == info['sha256'], path
    if info['sha256'] in rendered: continue
    rendered.add(info['sha256'])
    renderer = QSvgRenderer(str(path))
    assert renderer.isValid(), path
    for size in (16, 22, 24, 32):
        image = QImage(size, size, QImage.Format.Format_ARGB32_Premultiplied); image.fill(0)
        painter = QPainter(image);renderer.render(painter);painter.end()
        assert any(image.pixelColor(x,y).alpha() for y in range(size) for x in range(size)), (path,size)
        assert size != renderer.defaultSize().width() or not any(image.pixelColor(x,y).alpha() for x,y in
                       [(n,0) for n in range(size)] + [(n,size-1) for n in range(size)] +
                       [(0,n) for n in range(size)] + [(size-1,n) for n in range(size)]), ('clipped',path,size)

QIcon.setThemeSearchPaths([str(ROOT / 'themes/icons')])
checks = {'edit-find':'actions', 'configure':'actions', 'window-close':'actions',
          'audio-volume-high':'status', 'audio-volume-muted':'status', 'notifications':'status',
          'brightness-high':'status', 'bluetooth-symbolic':'devices', 'klipper-symbolic':'status',
          'format-text-bold':'actions', 'mail-reply-all':'actions', 'system-shutdown':'actions',
          'battery-050':'status', 'battery-050-charging':'status',
          'network-wireless-100':'status', 'printer-symbolic':'devices',
          'edit-copy-path':'actions','object-group':'actions',
          'phone-battery-medium':'status','network-mobile-60-5g-locked':'status',
          'network-wired-secure':'status','smartphone-trusted':'status'}
checks['speedometer-symbolic']='actions'
for name in ('SequoiaTahoe', 'SequoiaTahoe-light', 'SequoiaTahoe-dark'):
    QIcon.setThemeName(name)
    for icon, context in checks.items():
        assert QIcon.hasThemeIcon(icon), (name,icon)
        actual = QIcon.fromTheme(icon).pixmap(22,22).toImage()
        expected = QIcon(str(ROOT/'themes/icons'/name/context/'22'/(icon+'.svg'))).pixmap(22,22).toImage()
        # The base theme intentionally has different standalone fallback colors
        # for actions and tray status; KDE injects the live palette at runtime.
        if name == 'SequoiaTahoe':
            actual = actual.convertToFormat(QImage.Format.Format_Alpha8)
            expected = expected.convertToFormat(QImage.Format.Format_Alpha8)
        assert actual == expected, ('theme lookup resolves different artwork',name,icon)
    assert (ROOT/'themes/icons'/name/'COPYING.MaterialSymbols').is_file()
    for identity,source in material['protected_identities'][name].items():
        assert QIcon.hasThemeIcon(identity),(name,identity)
        expected=QIcon(str(ROOT/'themes/icons'/source))
        for size in (16,22,32,48,64):
            assert QIcon.fromTheme(identity).pixmap(size,size).toImage()==expected.pixmap(size,size).toImage(),('application identity shadowed',name,identity,size)
    for identity in ('view-app-grid','preferences-system','user-trash','user-trash-full'):
        assert identity in material['protected_identities'][name]
    # A scalable Actions glyph must not override large Documents/Desktop
    # folders. Preserve separate compact artwork, including at high DPI.
    for place in ('folder-documents', 'user-desktop', 'folder-tar', 'folder-bookmark'):
        for size in (16,22,24,32,48,64,128):
            folder = str(size) if size < 32 else 'scalable'
            source = ROOT/'themes/icons'/name/'places'/folder/(place+'.svg')
            if size < 32:
                action = ROOT/'themes/icons'/name/'actions'/folder/(place+'.svg')
                if action.exists(): source = action
            for scale in (1.0,2.0):
                actual = QIcon.fromTheme(place).pixmap(QSize(size,size),scale).toImage()
                expected = QIcon(str(source)).pixmap(QSize(size,size),scale).toImage()
                assert actual == expected, ('place size variant shadowed',name,place,size,scale)
        assert QIcon.hasThemeIcon(place+'-symbolic'), (name,place)
assert not any(Path(p).parts[1]=='apps' for p in material['generated']), 'Material artwork must never overwrite the Apps context'
print(f"PASS: {material['glyphs']} rounded glyphs / {len(material['generated'])} assets; checksums, unclipped 16/22/24/32px renders and native Qt theme lookups")

import tahoe_material as builder
from PySide6.QtCore import QByteArray
def state_image(symbol):
    renderer=QSvgRenderer(QByteArray(builder.svg(symbol,22,'#363636').encode()))
    image=QImage(22,22,QImage.Format.Format_ARGB32_Premultiplied); image.fill(0)
    painter=QPainter(image);renderer.render(painter);painter.end()
    return image
def distinct(symbols):
    images=[state_image(s) for s in symbols]
    assert all(a != b for i,a in enumerate(images) for b in images[i+1:]), symbols
distinct([f'sequoia_battery_{n}_{state}' for n in range(0,101,10) for state in ('normal','charging')]+['sequoia_battery_missing'])
distinct([f'sequoia_wifi_{n}_{badge}' for n in (0,20,40,60,80,100) for badge in ('normal','locked','limited')])
distinct(['sequoia_mic_low','sequoia_mic_medium','sequoia_mic_high'])
for device in ('keyboard','mouse','phone'):
    distinct([f'sequoia_peripheral_{device}_{level}' for level in (0,20,40,50,60,80,100)])
distinct([f'sequoia_cellular_{level}_{tech}_{badge}' for level in (0,20,40,60,80,100)
          for tech in ('none','5g','edge','gprs','hsdpa','hspa','hsupa','lte','umts') for badge in ('normal','locked')])
for device in ('smartphone','tablet','laptop'):
    distinct([f'sequoia_connection_{device}_{state}' for state in ('connected','disconnected','trusted')])
distinct(['sequoia_delete_row','sequoia_delete_column'])
distinct(['star','star_filled'])
distinct(['arrow_back','arrow_forward']); distinct(['format_list_numbered','format_list_numbered_rtl'])
for name,role in [('dialog-error','NegativeText'),('dialog-warning','NeutralText'),('data-success','PositiveText')]:
    assert builder.role_for(name)==role
    assert material['generated'][f'SequoiaTahoe-light/status/22/{name}.svg']['role']==role
print('PASS: battery/charging, Wi-Fi strength/lock/warning, microphone levels, filled states, directional icons and semantic color roles remain distinct')
print('PASS: peripheral batteries, mobile technologies, device trust and row/column controls remain distinct; protected app identities resolve to Tahoe at 16/22/32/48/64px')
