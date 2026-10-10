#!/usr/bin/env python3
"""Exercise native menu toggle painting, activation, RTL, and exclusive actions."""
import os
from pathlib import Path
import tempfile

os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
with tempfile.TemporaryDirectory(prefix='sequoia-menu-check-') as temporary:
    os.environ['XDG_CONFIG_HOME'] = temporary
    from PySide6.QtCore import QCoreApplication, Qt
    from PySide6.QtGui import QActionGroup, QColor, QPalette
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication, QMenu, QStyleFactory
    root = Path(__file__).resolve().parents[1]
    QCoreApplication.addLibraryPath(str(root / '.staging/desktop/usr/lib64/qt6/plugins'))
    app = QApplication([])
    style = QStyleFactory.create('SequoiaEleven')
    assert style, 'Build the native style first with tools/build-desktop.sh'
    app.setStyle(style)
    for mode, accent in [('light', '#2463d4'), ('dark', '#b45fe0')]:
        palette = app.palette()
        for group in (QPalette.Active, QPalette.Inactive, QPalette.Disabled):
            for role, color in [(QPalette.Window, '#252629' if mode == 'dark' else '#f2f3f5'),
                                (QPalette.WindowText, '#f2f3f5' if mode == 'dark' else '#24262b'),
                                (QPalette.Text, '#f2f3f5' if mode == 'dark' else '#24262b'),
                                (QPalette.Highlight, accent)]:
                palette.setColor(group, role, QColor(color))
        menu = QMenu()
        menu.setPalette(palette)
        on = menu.addAction('Show previews'); on.setCheckable(True); on.setChecked(True)
        off = menu.addAction('Show hidden files'); off.setCheckable(True)
        unavailable = menu.addAction('Unavailable'); unavailable.setCheckable(True); unavailable.setChecked(True); unavailable.setEnabled(False)
        menu.addSeparator()
        choices = QActionGroup(menu); choices.setExclusive(True)
        first = menu.addAction('Sort by name'); first.setCheckable(True); choices.addAction(first); first.setChecked(True)
        second = menu.addAction('Sort by date'); second.setCheckable(True); choices.addAction(second)
        menu.addMenu('More options').addAction('Details')
        for direction in (Qt.LeftToRight, Qt.RightToLeft):
            menu.setLayoutDirection(direction)
            menu.show(); app.processEvents()
            capture = menu.grab().toImage()
            rect = menu.actionGeometry(on)
            # Search the indicator column, clear of label glyphs and submenu arrows.
            x0 = rect.left() + 5 if direction == Qt.LeftToRight else rect.right() - 46
            matches = sum(capture.pixelColor(x, y).rgb() == QColor(accent).rgb()
                          for x in range(x0, x0 + 42)
                          for y in range(rect.top(), rect.bottom() + 1))
            assert matches > 100, (mode, direction, 'accent track missing', matches)
            if direction == Qt.LeftToRight:
                assert capture.save(str(root / 'preview' / f'menu-toggles-{mode}.png'))
            before = on.isChecked()
            QTest.mouseClick(menu, Qt.LeftButton, pos=menu.actionGeometry(on).center())
            assert on.isChecked() != before
            on.setChecked(True)
            menu.show(); app.processEvents()
            menu.setActiveAction(off)
            QTest.keyClick(menu, Qt.Key_Return)
            assert off.isChecked()
            off.setChecked(False)
            menu.show(); app.processEvents()
            QTest.mouseClick(menu, Qt.LeftButton, pos=menu.actionGeometry(unavailable).center())
            assert unavailable.isChecked()
            menu.show(); app.processEvents()
            QTest.mouseClick(menu, Qt.LeftButton, pos=menu.actionGeometry(second).center())
            assert second.isChecked() and not first.isChecked()
            first.setChecked(True)
            menu.hide()
    print('PASS: native menu accent painting, click/keyboard activation, disabled actions, exclusive groups, and RTL in both palettes.')
