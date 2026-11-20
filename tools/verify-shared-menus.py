#!/usr/bin/env python3
"""Exercise ordinary QMenus; no application-specific menu painter or proxy."""
import argparse
import configparser
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def check_live_updates(app, style):
    from PySide6.QtCore import QObject, QEvent, QPoint, QSize, Qt
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QLabel, QMenu, QWidgetAction
    import shiboken6

    class Events(QObject):
        def __init__(self, menu):
            super().__init__(); self.resizes = 0; self.hides = 0
            menu.installEventFilter(self)
        def eventFilter(self, obj, event):
            if event.type() == QEvent.Resize: self.resizes += 1
            if event.type() == QEvent.Hide: self.hides += 1
            return False

    menu = QMenu()
    first = menu.addAction('Initial action')
    embedded = QWidgetAction(menu)
    label = QLabel('Embedded control'); embedded.setDefaultWidget(label); menu.addAction(embedded)
    menu.popup(QPoint(140, 100)); QTest.qWait(80)
    events = Events(menu)
    original_min, original_max = menu.minimumSize(), menu.maximumSize()
    initial = menu.size()
    actions = [menu.addAction('A much longer dynamically discovered context action ' + str(i)) for i in range(9)]
    first.setText('Updated first action')
    assert menu.size() == initial, 'Action discovery exposed intermediate menu sizes'
    QTest.qWait(80)
    assert menu.isVisible() and events.hides == 0, 'Live updates remapped the popup'
    assert events.resizes == 1, f'Action batch resized {events.resizes} times'
    assert menu.width() > initial.width(), 'Live menu failed to grow'
    assert label.isVisible() and embedded.defaultWidget() == label, 'Embedded widget action was lost'
    assert menu.minimumSize() == original_min and menu.maximumSize() == original_max
    events.resizes = 0
    for action in actions: menu.removeAction(action)
    first.setText('Short')
    QTest.qWait(80)
    assert menu.width() < initial.width() + 10 and events.resizes == 1, 'Menu did not shrink to its final contents'
    assert menu.actionGeometry(embedded).bottom() < menu.height(), 'Final action layout is clipped'
    menu.addAction('Pending change'); menu.hide(); QTest.qWait(30)
    assert menu.updatesEnabled() and menu.minimumSize() == original_min and menu.maximumSize() == original_max
    menu.popup(QPoint(140,100)); QTest.qWait(40)
    menu.addAction('Pending style replacement')
    style.unpolish(menu)
    assert menu.updatesEnabled() and menu.maximumSize() == original_max, 'Unpolish left menu frozen'
    style.polish(menu); menu.hide(); menu.popup(QPoint(140,100)); QTest.qWait(40)
    menu.addAction('Pending destruction'); shiboken6.delete(menu); QTest.qWait(30)

    parent = QMenu(); child = parent.addMenu('Dynamic submenu'); child.addAction('Small')
    parent.popup(QPoint(140,100)); QTest.qWait(60)
    parent.setActiveAction(child.menuAction())
    QTest.keyClick(parent, Qt.Key_Left if app.layoutDirection() == Qt.RightToLeft else Qt.Key_Right)
    QTest.qWait(100)
    child_events = Events(child)
    for i in range(5): child.addAction('A wider submenu action ' + str(i))
    QTest.qWait(80)
    a,b = parent.geometry(), child.geometry()
    assert child.isVisible() and child_events.hides == 0 and child_events.resizes == 1
    assert max(b.left()-a.right()-1,a.left()-b.right()-1) >= 6, 'Growing submenu lost its surface gap'
    QTest.keyClick(child, Qt.Key_Escape); QTest.keyClick(parent, Qt.Key_Escape)
    assert not child.isVisible() and not parent.isVisible(), 'Updated menus lost keyboard dismissal'


def render(args):
    from PySide6.QtCore import QPoint, Qt
    from PySide6.QtGui import QAction, QActionGroup, QColor, QIcon, QKeySequence, QPalette
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication, QMenu, QStyle, QStyleFactory
    app = QApplication([])
    style = QStyleFactory.create('SequoiaElevenGlass' if args.material == 'glass' else 'SequoiaEleven')
    assert style and style.property('sequoiaSharedMenus'), 'Shared native style was not loaded'
    app.setStyle(style)
    palette = app.palette()
    cfg = configparser.ConfigParser(); cfg.optionxform = str
    cfg.read(Path(os.environ['XDG_CONFIG_HOME'])/'kdeglobals')
    for role, section, key in [(QPalette.Window, 'Colors:Window', 'BackgroundNormal'),
                               (QPalette.WindowText, 'Colors:Window', 'ForegroundNormal'),
                               (QPalette.Text, 'Colors:View', 'ForegroundNormal'),
                               (QPalette.Highlight, 'Colors:Selection', 'BackgroundNormal')]:
        palette.setColor(role, QColor(*map(int, cfg[section][key].split(','))))
    palette.setColor(QPalette.Base, palette.color(QPalette.Window))
    palette.setColor(QPalette.Button, palette.color(QPalette.Window))
    palette.setColor(QPalette.ButtonText, palette.color(QPalette.WindowText))
    palette.setColor(QPalette.Disabled, QPalette.Text, QColor(128,128,128))
    palette.setColor(QPalette.Disabled, QPalette.WindowText, QColor(128,128,128))
    app.setPalette(palette)
    if args.rtl: app.setLayoutDirection(Qt.RightToLeft)
    QIcon.setThemeName('breeze-dark' if args.dark else 'breeze')
    menu = QMenu()
    toggle = menu.addAction(QIcon.fromTheme('view-fullscreen'), 'Full Screen Mode')
    toggle.setCheckable(True); toggle.setShortcut(QKeySequence('Ctrl+Shift+F'))
    menu.addSeparator()
    menu.addAction(QIcon.fromTheme('go-previous'), 'Previous')
    menu.addAction(QIcon.fromTheme('go-next'), 'Next')
    menu.addSeparator()
    group = QActionGroup(menu); group.setExclusive(True)
    actual = menu.addAction(QIcon.fromTheme('zoom-original'), 'Zoom to Actual Size')
    fit = menu.addAction(QIcon.fromTheme('zoom-fit-best'), 'Zoom to Fit')
    for action in (actual, fit): action.setCheckable(True); group.addAction(action)
    fit.setChecked(True)
    menu.addSeparator()
    submenu = QMenu(menu)
    # KDE KActionMenu owns an action separate from QMenu.menuAction().
    branch = QAction(QIcon.fromTheme('color-management'), 'Background Color Mode', menu)
    branch.setMenu(submenu); menu.addAction(branch)
    colors = QActionGroup(submenu); colors.setExclusive(True)
    for label in ('Follow color scheme', 'Light Mode', 'Neutral Mode', 'Dark Mode'):
        action = submenu.addAction(label); action.setCheckable(True); colors.addAction(action)
    colors.actions()[0].setChecked(True)
    menu.addSeparator()
    copy = menu.addAction(QIcon.fromTheme('edit-copy'), 'Copy'); copy.setShortcut(QKeySequence.Copy)
    disabled = menu.addAction('Unavailable action'); disabled.setEnabled(False)
    menu.popup(QPoint(120, 80)); QTest.qWait(150)
    menu.setActiveAction(branch)
    QTest.keyClick(menu, Qt.Key_Left if args.rtl else Qt.Key_Right); QTest.qWait(250)
    assert submenu.isVisible(), 'Keyboard navigation did not open submenu'
    parent_rect, child_rect = menu.geometry(), submenu.geometry()
    gap = max(child_rect.left() - parent_rect.right() - 1, parent_rect.left() - child_rect.right() - 1)
    assert gap >= 6, f'Submenu overlaps its parent: gap={gap}, parent={parent_rect}, child={child_rect}'
    image = menu.grab().toImage()
    assert image.pixelColor(0, 0).alpha() == 0, 'Popup corners were not transparent'
    expected = palette.color(QPalette.Window).alpha()
    assert abs(image.pixelColor(menu.width()//2, 3).alpha() - expected) <= 1, 'Menu material alpha changed'
    dpr = image.devicePixelRatio()
    edge = int(24 * dpr)
    partial = [image.pixelColor(x,y).alpha() for y in range(edge) for x in range(edge)
               if 0 < image.pixelColor(x,y).alpha() < expected]
    assert len(partial) >= 12, 'Rounded edge lost antialiased alpha coverage'
    assert menu.actionGeometry(toggle).left() >= 8
    assert menu.actionGeometry(toggle).top() >= 8
    assert menu.actionGeometry(toggle).height() >= 30
    stem = ('dark' if args.dark else 'light') + '-' + args.material + ('-rtl' if args.rtl else '')
    image.save(str(args.output/(stem+'.png')))
    submenu.grab().save(str(args.output/(stem+'-submenu.png')))
    QTest.keyClick(submenu, Qt.Key_Escape); QTest.qWait(30)
    assert not submenu.isVisible() and menu.isVisible(), 'Escape did not return to parent menu'
    menu.setActiveAction(toggle); QTest.keyClick(menu, Qt.Key_Return); QTest.qWait(30)
    assert toggle.isChecked(), 'Native toggle did not activate'
    menu.popup(QPoint(120,80)); menu.setActiveAction(actual)
    QTest.keyClick(menu, Qt.Key_Return); QTest.qWait(30)
    assert actual.isChecked() and not fit.isChecked(), 'Exclusive actions did not remain exclusive'
    menu.popup(QPoint(120,80)); QTest.qWait(30)
    QTest.mouseClick(menu, Qt.LeftButton, Qt.NoModifier, menu.actionGeometry(disabled).center())
    assert menu.isVisible(), 'Disabled action dismissed the menu'
    menu.close()
    check_live_updates(app, style)
    print('PASS:', stem, 'native submenu, Escape, toggles, radio actions, disabled actions, padding, separated submenus, antialiased corners, material and live action batches')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plugins', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--child', action='store_true')
    parser.add_argument('--dark', action='store_true')
    parser.add_argument('--rtl', action='store_true')
    parser.add_argument('--material', choices=('solid','transparent','glass'), default='solid')
    args = parser.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    if args.child: render(args)
    else:
        for mode, material, rtl in [(m,s,False) for m in ('Light','Dark') for s in ('solid','transparent','glass')] + [('Dark','solid',True)]:
            with tempfile.TemporaryDirectory(prefix='shared-menu-test-') as temp:
                cfg = configparser.ConfigParser(); cfg.optionxform = str
                cfg.read(ROOT/f'themes/color-schemes/SequoiaEleven{mode}{"Solid" if material=="solid" else "Transparent"}.colors')
                alpha = 255 if material=='solid' else (168 if mode=='Dark' else 148) if material=='glass' else 192 if mode=='Dark' else 184
                rgb = cfg['Colors:Window']['BackgroundNormal'].split(',')[:3]
                cfg['Colors:Window']['BackgroundNormal'] = ','.join(rgb+[str(alpha)])
                with (Path(temp)/'kdeglobals').open('w') as stream: cfg.write(stream, space_around_delimiters=False)
                screen = Path(temp)/'screen.json'
                screen.write_text(json.dumps({'screens':[{'name':'menus','width':2400,'height':1800,'logicalDpi':96,'logicalBaseDpi':96,'dpr':1}]}))
                env = dict(os.environ, XDG_CONFIG_HOME=temp, XDG_CONFIG_DIRS=temp, QT_QPA_PLATFORM='offscreen:configfile='+str(screen),
                           QT_QPA_PLATFORMTHEME='generic', QT_PLUGIN_PATH=str(args.plugins.resolve()), QT_STYLE_OVERRIDE='Fusion')
                subprocess.run([sys.executable, __file__, '--child', '--plugins', str(args.plugins), '--output', str(args.output),
                                '--material', material, *(['--dark'] if mode=='Dark' else []), *(['--rtl'] if rtl else [])], env=env, check=True)
