#!/usr/bin/env python3
"""Local motion comparison lab. Never changes desktop animation settings.

Run normally to explore; --check validates interaction, motion, and reduced motion
in an offscreen window and saves a screenshot under preview/.
"""
import argparse
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'tools/motion-lab'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
# Use self-contained Qt controls; do not load the desktop's style overlays.
os.environ['QT_QUICK_CONTROLS_STYLE'] = 'Basic'
os.environ['QT_QUICK_BACKEND'] = 'software'
for key in ('QML_IMPORT_PATH', 'QT_PLUGIN_PATH', 'QT_QUICK_CONTROLS_CONF'):
    os.environ.pop(key, None)
if args.check:
    os.environ['QT_QPA_PLATFORM'] = 'offscreen'

from PySide6.QtCore import QObject, QUrl, Slot
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuick import QQuickWindow


class CandidateStore(QObject):
    def __init__(self):
        super().__init__()
        self.directory = ROOT / 'preview/motion-palettes'

    @Slot(str, result=str)
    def save(self, payload):
        try:
            candidate = json.loads(payload)
            if (len(candidate['durations']) != 7
                    or any(type(n) not in (int, float) or not 0 <= n <= 1000 for n in candidate['durations'])
                    or candidate['curve'] not in ('OutCubic', 'OutQuint')):
                raise ValueError('Invalid motion palette')
            directory = self.directory
            directory.mkdir(parents=True, exist_ok=True)
            # Each save keeps previous candidates for comparison.
            from datetime import datetime
            path = directory / ('candidate-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.json')
            path.write_text(json.dumps(candidate, indent=2, ensure_ascii=False) + '\n')
            return 'Saved ' + str(path)
        except (OSError, ValueError, KeyError, TypeError) as error:
            return 'Could not save: ' + str(error)


app = QGuiApplication(sys.argv[:1])
app.setApplicationName('Sequoia Motion Lab')
engine = QQmlApplicationEngine()
store = CandidateStore()
engine.rootContext().setContextProperty('motionData', json.loads((ASSETS / 'palettes.json').read_text()))
engine.rootContext().setContextProperty('candidateStore', store)
qml_errors = []
engine.warnings.connect(lambda warnings: qml_errors.extend(str(w) for w in warnings))
engine.load(QUrl.fromLocalFile(str(ASSETS / 'Main.qml')))
if not engine.rootObjects():
    raise SystemExit('Motion Lab failed to load')
window = engine.rootObjects()[0]

if args.check:
    from PySide6.QtTest import QTest
    from PySide6.QtCore import QMetaObject, Qt, Q_RETURN_ARG
    QTest.qWait(250)
    def invoke(obj, method):
        assert QMetaObject.invokeMethod(obj, method, Qt.DirectConnection), method
    for category in range(7):
        window.setProperty('categoryIndex', category)
        for preset in range(3):
            panel = window.findChild(QObject, 'paletteA')
            panel.setProperty('presetIndex', preset)
            sample = window.findChild(QObject, 'sampleA')
            invoke(sample, 'reset')
            invoke(sample, 'play')
            QTest.qWait(35)
            assert 0 < sample.property('progress') < 1, (category, preset, 'no intermediate animation')
            QTest.qWait(750)
            assert abs(sample.property('progress') - 1) < .01, (category, preset, 'animation did not settle')
            if category in (1, 6):
                assert abs(sample.property('pulse')) < .01, 'attention did not stop'
            invoke(sample, 'play')
            QTest.qWait(750)
            assert sample.property('progress') < .01, 'reverse transition failed'
    window.setProperty('reducedMotion', True)
    for category in range(7):
        window.setProperty('categoryIndex', category)
        sample = window.findChild(QObject, 'sampleA')
        invoke(sample, 'reset')
        invoke(sample, 'play')
        QTest.qWait(150)
        assert sample.property('travel') == 0, 'reduced motion still translates'
        assert sample.property('pulse') == 0, 'reduced motion still pulses'
        assert sample.property('progress') > .99, 'reduced motion failed to settle'
    window.setProperty('reducedMotion', False)
    window.setProperty('categoryIndex', 2)
    window.findChild(QObject, 'paletteA').setProperty('presetIndex', 0)
    window.findChild(QObject, 'paletteB').setProperty('presetIndex', 1)
    exported = QMetaObject.invokeMethod(window.findChild(QObject, 'paletteA'),
        'exportCandidate', Qt.DirectConnection, Q_RETURN_ARG('QVariant'))
    candidate = json.loads(exported)
    assert len(candidate['durations']) == len(candidate['categories']) == 7
    assert candidate['name'] == 'Precise' and not candidate['reducedMotion']
    assert 'playbackScale' not in candidate
    import tempfile
    with tempfile.TemporaryDirectory(prefix='sequoia-motion-export-') as temporary:
        store.directory = Path(temporary)
        assert store.save(exported).startswith('Saved ')
        assert store.save(exported).startswith('Saved ')
        files = list(store.directory.glob('*.json'))
        assert len(files) == 2, 'saving overwrote a prior candidate'
        assert all(json.loads(path.read_text()) == candidate for path in files)
    invoke(window, 'replayBoth')
    QTest.qWait(800)
    path = ROOT / 'preview/motion-lab.png'
    assert window.grabWindow().save(str(path))
    assert not qml_errors, '\n'.join(qml_errors)
    print('PASS: seven families × three palettes; forward/reverse motion, finite attention, reduced motion; no QML warnings')
    print('Preview:', path)
else:
    sys.exit(app.exec())
