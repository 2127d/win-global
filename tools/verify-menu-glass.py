#!/usr/bin/env python3
"""Check rendered glass, live accent changes, and disabled native menu rows."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_QUICK_BACKEND='software', QT_STYLE_OVERRIDE='Fusion',
                  QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
with tempfile.TemporaryDirectory(prefix='sequoia-menu-glass-') as temporary:
    os.environ['XDG_CONFIG_HOME'] = temporary
    from PySide6.QtCore import QRect, Qt
    from PySide6.QtGui import QColor, QImage, QPainter, QPalette
    from PySide6.QtWidgets import QApplication, QStyle, QStyleFactory, QStyleOptionMenuItem
    app = QApplication([])
    app.addLibraryPath(str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'))
    for name in ('SequoiaEleven', 'SequoiaElevenGlass'):
        style = QStyleFactory.create(name)
        assert style, name
        for dark in (False, True):
            option = QStyleOptionMenuItem()
            option.rect = QRect(0, 0, 240, 36)
            option.menuItemType = QStyleOptionMenuItem.Normal
            option.state = QStyle.State_Enabled | QStyle.State_Selected
            option.palette.setColor(QPalette.Window, QColor('#252629' if dark else '#f2f3f5'))
            def render():
                image = QImage(240, 36, QImage.Format_ARGB32_Premultiplied)
                image.fill(Qt.transparent)
                painter = QPainter(image)
                style.drawControl(QStyle.CE_MenuItem, option, painter)
                painter.end()
                return image
            for accent in ('#df3040', '#3060df'):
                option.palette.setColor(QPalette.Highlight, QColor(accent))
                image = render()
                center = image.pixelColor(120, 18)
                assert 0 < center.alpha() < 160, (name, dark, center)
                assert (center.red() > center.blue()) == (accent == '#df3040'), center
                assert image.pixelColor(120, 5) != image.pixelColor(120, 29), (name, dark, 'Missing glass sheen')
                assert image.pixelColor(0, 0).alpha() == 0, 'Glass escapes rounded bounds'
            option.state = QStyle.State_Selected
            assert render().pixelColor(120, 18).alpha() == 0, 'Disabled row gets a highlight'
    fixture = Path(temporary) / 'tst_glass.qml'
    fixture.write_text('''import QtQuick
import QtTest
import org.kde.kirigami as K
import org.kde.ksvg as KSvg
import "SOURCE" as Popup
Item {
    width: 240; height: 80
    Popup.MenuHighlight {
        id: row; width: 220; height: 36
        K.Theme.inherit: false
        K.Theme.backgroundColor: "#f2f3f5"
    }
    KSvg.FrameSvgItem {
        id: plasmaRow; width: 220; height: 36; y: 40
        imagePath: "PLASMA"
        prefix: "hover"
        K.Theme.inherit: false
        K.Theme.highlightColor: row.K.Theme.highlightColor
    }
    TestCase {
        name: "MenuGlassAccent"; when: windowShown
        function test_liveAccent_data() { return [{tag:"light",dark:false},{tag:"dark",dark:true}]; }
        function test_liveAccent(data) {
            row.K.Theme.backgroundColor = data.dark ? "#252629" : "#f2f3f5";
            row.K.Theme.highlightColor = "#df3040";
            waitForRendering(row); wait(50);
            const red = grabImage(row);
            verify(red.red(110,18) > red.blue(110,18));
            verify(red.red(110,5) !== red.red(110,29), "Glass has a vertical sheen");
            row.K.Theme.highlightColor = "#3060df";
            waitForRendering(row); wait(50);
            const blue = grabImage(row);
            verify(blue.blue(110,18) > blue.red(110,18), "Open highlight tracks new accent");
            waitForRendering(plasmaRow); wait(50);
            const plasma = grabImage(plasmaRow);
            verify(plasma.blue(110,18) > plasma.red(110,18), "Plasma resolves the live accent role");
            row.K.Theme.highlightColor = "#df3040";
            waitForRendering(plasmaRow); wait(50);
            const plasmaRed = grabImage(plasmaRow);
            verify(plasmaRed.red(110,18) > plasmaRed.blue(110,18), "Plasma follows accent changes");
            row.focused = true;
            compare(row.border.color, row.K.Theme.highlightColor);
            row.pressed = true;
            verify(row.color.a > 0.25);
            row.pressed = false; row.focused = false;
        }
    }
}
'''.replace('SOURCE', (ROOT / 'desktop/components/popups').as_uri()).replace('PLASMA', str(ROOT / 'themes/plasma/desktoptheme/sequoia-eleven-light/widgets/viewitem.svg')))
    subprocess.run(['/usr/lib64/qt6/bin/qmltestrunner', '-input', str(fixture), '-o', '-,txt'], check=True)
print('PASS: native and QML glass sheen, translucency, live accent, focus, pressed, and disabled states')
