#!/usr/bin/env python3
"""Render real native menus across material changes in isolated configuration."""
import configparser
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def child(style_name):
    from PySide6.QtCore import QMetaObject, QRect, Qt
    from PySide6.QtGui import QColor, QImage, QPainter, QPalette
    from PySide6.QtWidgets import QApplication, QMenu, QStyle, QStyleFactory, QStyleOption
    spec = importlib.util.spec_from_file_location('material', ROOT/'apps/settings/appearance-material.py')
    material = importlib.util.module_from_spec(spec); spec.loader.exec_module(material)
    app = QApplication([])
    style = QStyleFactory.create(style_name)
    assert style, style_name
    menu = QMenu(); menu.setStyle(style)
    menu.setAttribute(Qt.WA_TranslucentBackground)

    def alpha():
        palette = menu.palette()
        rgba = [int(v) for v in material.read('kdeglobals', 'Colors:Window', 'BackgroundNormal').split(',')]
        palette.setColor(QPalette.Window, QColor(*rgba))
        menu.setPalette(palette)
        option = QStyleOption(); option.initFrom(menu); option.rect = QRect(0, 0, 240, 140)
        image = QImage(240, 140, QImage.Format_ARGB32_Premultiplied); image.fill(QColor('transparent'))
        painter = QPainter(image)
        style.drawPrimitive(QStyle.PE_PanelMenu, option, painter, menu); painter.end()
        return image.pixelColor(120, 70).alpha()

    assert alpha() < 255, 'Starting Transparent scheme did not reproduce translucent menus'
    for surface in ('solid', 'glass', 'solid', 'transparent', 'solid'):
        # Use the real Hub recipe, without sending changes to the live desktop.
        changes = material.plan(surface)
        files = {}
        for entry in changes:
            path = material.CONFIG/entry['file']
            if path not in files:
                cfg = configparser.ConfigParser(interpolation=None, strict=False); cfg.optionxform = str; cfg.read(path)
                files[path] = cfg
            cfg = files[path]
            if not cfg.has_section(entry['group']): cfg.add_section(entry['group'])
            cfg.set(entry['group'], entry['key'], entry['after'])
        for path, cfg in files.items():
            with path.open('w') as stream: cfg.write(stream, space_around_delimiters=False)
        assert QMetaObject.invokeMethod(style, 'configurationChanged', Qt.DirectConnection)
        actual = alpha()
        assert (actual == 255) == (surface == 'solid'), (style_name, surface, actual)
    print(f'PASS: {style_name} menus follow repeated Solid/Glass/Transparent changes in the same process')


if __name__ == '__main__':
    if len(sys.argv) > 1:
        child(sys.argv[1])
    else:
        for mode in ('Light', 'Dark'):
            for style in ('SequoiaEleven', 'SequoiaElevenGlass'):
                with tempfile.TemporaryDirectory(prefix='material-menu-test-') as temp:
                    base = Path(temp)
                    (base/'kdeglobals').write_text((ROOT/f'themes/color-schemes/SequoiaEleven{mode}Transparent.colors').read_text()
                                                 + f'\n[General]\nColorScheme=SequoiaEleven{mode}Transparent\n')
                    (base/'sequoiaelevenrc').write_text('[Style]\nMenuOpacity=44\n')
                    env = dict(os.environ, XDG_CONFIG_HOME=temp, XDG_CONFIG_DIRS=temp,
                               QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
                    subprocess.run([sys.executable, __file__, style], env=env, check=True)
