#!/usr/bin/env python3
"""Check KDE background variants and reproduce the laptop's missing binding."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import quick_islands
from quick_islands import background


class BackgroundTests(unittest.TestCase):
    canvas = '    background: Sequoia.Canvas {}'

    def test_explicit_background_preserves_following_members(self):
        for whitespace in ('    ', '\t', '  '):
            with self.subTest(whitespace=whitespace):
                source = '''QQC2.Page {
    id: root
    // background: Rectangle { example only }
    Item { background: Rectangle {} }
''' + whitespace + '''background:
    Rectangle {
        property string braces: "} {"
        /* } */
        Rectangle {}
    }
    transitions: Transition {}
}
'''
                result = background(source, self.canvas)
                self.assertIn(self.canvas, result)
                self.assertIn('Item { background: Rectangle {} }', result)
                self.assertIn('transitions: Transition {}', result)
                self.assertNotIn('property string braces', result)

    def test_inherited_page_background(self):
        source = '''QQC2.Page {
    id: root
    Item { background: Rectangle {} }
    onVisibleChanged: console.log("preserved")
}
'''
        result = background(source, self.canvas, inherited_root='QQC2.Page')
        self.assertIn('id: root\n\n' + self.canvas, result)
        self.assertIn('Item { background: Rectangle {} }', result)
        self.assertIn('onVisibleChanged: console.log("preserved")', result)

    def test_unsupported_backgrounds_fail_clearly(self):
        for source in ('QQC2.Page {\n    id: root\n    background: null\n}',
                       'QQC2.Page {\n    id: root\n    background: factory()\n}',
                       'QQC2.Page {\n    id: root\n    background: Rectangle {',
                       'OtherPage {\n    id: root\n}'):
            with self.subTest(source=source), self.assertRaises(RuntimeError):
                background(source, self.canvas, inherited_root='QQC2.Page')
        with self.assertRaises(RuntimeError):
            background('QQC2.Page {\n    id: root\n}', self.canvas)

    def test_overlay_with_inherited_page_and_failed_refresh(self):
        with tempfile.TemporaryDirectory(prefix='sequoia-background-check-') as temporary:
            base = Path(temporary)
            sources = base / 'system-qml'
            for module in quick_islands.MODULES:
                shutil.copytree(quick_islands.QML / module, sources / module,
                                copy_function=os.symlink, symlinks=True)
            relative = Path('org/kde/kirigami/controls/Page.qml')
            page = sources / relative
            original = page.read_text()
            inherited = background(original, '')
            page.unlink()  # Replace the test symlink; never edit system QML.
            page.write_text(inherited)
            output = base / 'overlay'
            with patch.object(quick_islands, 'QML', sources):
                quick_islands.build(output)
                result = (output / relative).read_text()
                self.assertIn('background: Sequoia.Canvas {', result)
                self.assertIn('solidColor: root.Platform.Theme.backgroundColor', result)
                manifest = (output / 'manifest.json').read_bytes()
                self.assertEqual(json.loads(manifest)['sources'][str(page)],
                                 hashlib.sha256(page.read_bytes()).hexdigest())
                # Load the generated page with Qt, catching invalid QML or a
                # misplaced binding that string assertions would miss.
                probe = '''
import sys
from pathlib import Path
from PySide6.QtCore import QUrl
from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine
from PySide6.QtQuick import QQuickItem
app = QApplication([])
engine = QQmlEngine()
engine.addImportPath(sys.argv[1])
component = QQmlComponent(engine, QUrl.fromLocalFile(str(
    Path(sys.argv[1]) / 'org/kde/kirigami/controls/Page.qml')))
page = component.create()
assert page is not None, [error.toString() for error in component.errors()]
assert page.property('background').objectName() == 'sequoiaCanvas'
'''
                subprocess.run([sys.executable, '-c', probe, str(output)], check=True, timeout=30,
                               env=dict(os.environ, QT_QPA_PLATFORM='offscreen',
                                        QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                                        QT_QUICK_CONTROLS_STYLE='org.kde.desktop'))
                page.write_text(inherited.replace('QQC2.Page {', 'UnknownPage {', 1))
                with self.assertRaisesRegex(RuntimeError, r'controls/Page.qml:'):
                    quick_islands.build(output)
                self.assertEqual((output / relative).read_text(), result)
                self.assertEqual((output / 'manifest.json').read_bytes(), manifest)
                self.assertEqual(list(base.glob('.quick-build-*')), [])


if __name__ == '__main__':
    unittest.main()
