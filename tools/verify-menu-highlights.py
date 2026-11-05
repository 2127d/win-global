#!/usr/bin/env python3
"""Check the actual packaged menu delegates under the user's Qt Quick desktop style."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from preview_session import ensure_private_session

ensure_private_session()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--packages', type=Path, default=Path.home() / '.local/share/plasma/plasmoids')
parser.add_argument('--style-plugins', type=Path, default=Path.home() / '.local/share/sequoia-eleven/downloads-style')
args = parser.parse_args()
qt_root = Path('/usr/lib64/qt6/bin')
families = (
    ('volume', 'AudioMenu', 'ui'),
    ('brightness', 'DisplayMenu', 'ui'),
    ('networkmanagement', 'NetworkMenu', 'ui'),
    ('clipboard', 'ClipboardActionMenu', 'ui/views'),
    ('notifications', 'NotificationDurationMenu', 'ui/components'),
)
with tempfile.TemporaryDirectory(prefix='sequoia-menu-bounds-') as directory:
    fixture = Path(directory)
    imports, components, cases = [], [], []
    for index, (plugin, menu, path) in enumerate(families):
        folder = args.packages.resolve() / ('org.sequoia.' + plugin) / 'contents' / path
        imports.append(f'import "{folder.as_uri()}" as Family{index}')
        # The long row forces horizontal elision. Enough real delegates to exercise vertical scrolling.
        rows = '\n'.join(f'Family{index}.{menu}Item {{ text: "{label}" }}' for label in
                         ['First', 'An unusually long device or action name ' * 4, 'Three', 'Four', 'Five', 'Six', 'Seven', 'Last'])
        components.append(f'Component {{ id: family{index}; Family{index}.{menu} {{ {rows} }} }}')
        for small in (False, True):
            cases.append(f'{{ tag: "{plugin}-{"scroll" if small else "full"}", component: family{index}, small: {str(small).lower()} }}')
    test = '''import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Controls as QQC
import org.kde.plasma.components as PC
IMPORTS
Item {
    id: host
    width: 420; height: 500
    COMPONENTS
    Component { id: desktopSelector; QQC.ComboBox { width: 180; model: ["First", "Second", "Third"] } }
    Component { id: plasmaSelector; PC.ComboBox { width: 180; implicitHeight: 36; model: ["First", "Second", "Third"] } }
    Component { id: audioSelector; Family0.AudioDevicePicker {
        width: 180
        devices: [{id:"a",name:"Output A"}, {id:"b",name:"Output B"}, {id:"c",name:"Output C"}]
        record: devices[0]
        controller: ({})
    } }
    TestCase {
        name: "MenuContainment"
        when: windowShown
        function test_selectors_data() { return [
            { tag: "audio-routing", component: audioSelector, inset: 8 },
            { tag: "plasma-selector", component: plasmaSelector, inset: 4 },
            { tag: "desktop-settings-selector", component: desktopSelector, inset: 4 },
        ]; }
        function test_selectors(data) {
            host.Window.window.width = 340;
            host.Window.window.height = 300;
            const picker = createTemporaryObject(data.component, host);
            verify(picker);
            picker.forceActiveFocus();
            mouseClick(picker, picker.width / 2, picker.height / 2);
            tryCompare(picker.popup, "opened", true);
            const list = picker.popup.contentItem;
            const surface = picker.popup.background;
            for (let i = 0; i < picker.count; ++i) {
                tryVerify(() => list.itemAtIndex(i) !== null);
                const row = list.itemAtIndex(i);
                mouseMove(row, row.width / 2, row.height / 2);
                const box = row.mapToItem(surface, 0, 0, row.width, row.height);
                verify(box.x >= data.inset - 0.5 && box.y >= data.inset - 0.5, "leading inset");
                verify(box.x + box.width <= surface.width - data.inset + 0.5, "trailing inset");
                verify(box.y + box.height <= surface.height - data.inset + 0.5, "bottom inset");
            }
            keyClick(Qt.Key_Escape);
            tryCompare(picker.popup, "visible", false);
        }
        function test_highlights_data() { return [CASES]; }
        function test_highlights(data) {
            const window = host.Window.window;
            window.width = data.small ? 230 : 420;
            window.height = data.small ? 184 : 500;
            const menu = createTemporaryObject(data.component, host);
            verify(menu);
            menu.popup(host, 4, 4);
            tryCompare(menu, "opened", true);
            const list = menu.contentItem;
            const surface = menu.background;
            const bounds = item => item.mapToItem(surface, 0, 0, item.width, item.height);
            const inside = box => box.x >= 7.5 && box.y >= 7.5
                && box.x + box.width <= surface.width - 7.5 && box.y + box.height <= surface.height - 7.5;
            verify(inside(bounds(list)), "List viewport exceeds the inset");
            compare(list.interactive, data.small);
            for (let i = 0; i < menu.count; ++i) {
                list.positionViewAtIndex(i, ListView.Contain);
                const row = menu.itemAt(i);
                mouseMove(row, row.width / 2, row.height / 2);
                tryCompare(row, "hovered", true);
                verify(inside(bounds(row.background)), "Hover exceeds frame at row " + i);
            }
            menu.close();
            tryCompare(menu, "visible", false);
            mouseMove(host, 0, 0);
            list.positionViewAtBeginning();
            menu.popup(host, 4, 4);
            tryCompare(menu, "opened", true);
            menu.currentIndex = -1;
            for (let i = 0; i < menu.count; ++i) {
                keyClick(Qt.Key_Down);
                tryCompare(menu, "currentIndex", i);
                const row = menu.itemAt(i);
                tryVerify(() => row.highlighted && inside(bounds(row.background)), 1000, "Keyboard row " + i);
            }
            keyClick(Qt.Key_Escape);
            tryCompare(menu, "visible", false);
        }
    }
}
'''
    (fixture / 'tst_bounds.qml').write_text(test.replace('IMPORTS', '\n'.join(imports))
        .replace('COMPONENTS', '\n'.join(components)).replace('CASES', ','.join(cases)))
    config = Path(os.environ['XDG_CONFIG_HOME'])
    for name in ('kdeglobals', 'plasmarc'):
        original = Path.home() / '.config' / name
        if original.exists():
            shutil.copy2(original, config / name)
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
               QT_QPA_PLATFORMTHEME='kde', QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
               QML_IMPORT_PATH=str(Path.home() / '.local/share/sequoia-eleven/motion/qml'))
    env.update(QT_STYLE_OVERRIDE='SequoiaEleven', QT_PLUGIN_PATH=str(args.style_plugins.resolve()))
    subprocess.run([qt_root / 'qmltestrunner', '-input', fixture, '-o', '-,txt'], env=env, check=True)

    # QMenu can give a long action more width than its visible popup. Compare
    # actual pixels so checking only the reported action rectangle cannot pass.
    os.environ.update(env)
    from PySide6.QtCore import QPoint
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication, QMenu
    app = QApplication([])
    assert app.style().objectName().lower() == 'sequoiaeleven'
    menu = QMenu()
    long_action = menu.addAction('Long device name ' * 18)
    menu.addAction('Short name')
    menu.setFixedWidth(218)
    menu.popup(QPoint(20, 20))
    QTest.qWait(100)
    menu.setActiveAction(None)
    QTest.qWait(200)
    before = menu.grab().toImage()
    menu.setActiveAction(long_action)
    QTest.qWait(200)
    after = menu.grab().toImage()
    scale = before.devicePixelRatio()
    y = round(menu.actionGeometry(long_action).center().y() * scale)
    inset = round(4 * scale)
    changed = [x for x in range(before.width()) if before.pixel(x, y) != after.pixel(x, y)]
    assert changed and min(changed) >= inset and max(changed) < before.width() - inset, changed
    menu.close()
    print('PASS: native highlight stays inside the frame for an overwide action')
