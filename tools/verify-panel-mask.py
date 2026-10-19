#!/usr/bin/env python3
"""Check Plasma's actual panel blur regions remain inside the rounded border."""
import os
from pathlib import Path
import tempfile

temporary = tempfile.TemporaryDirectory(prefix='sequoia-panel-mask-')
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                  XDG_CACHE_HOME=temporary.name)

from PySide6.QtCore import QPoint, QRect, QTimer, QUrl
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlComponent, QQmlEngine

ROOT = Path(__file__).resolve().parents[1]
app = QGuiApplication([])
engine = QQmlEngine()
items = []
components = []
for path in sorted((ROOT / 'themes/plasma/desktoptheme').glob('sequoia-eleven-*/**/panel-background.svg')):
    for width, height in ((977, 60), (1920, 36), (60, 300)):
        component = QQmlComponent(engine)
        component.setData(('import QtQuick\nimport org.kde.ksvg as KSvg\n'
                           'KSvg.FrameSvgItem { width: %d; height: %d; imagePath: "%s" }'
                           % (width, height, path)).encode(), QUrl())
        item = component.create()
        if not item:
            raise RuntimeError(component.errors())
        components.append(component)
        items.append((path, item, width, height))


def verify():
    try:
        for path, item, width, height in items:
            mask = item.property('mask')
            assert not mask.isEmpty(), (path, 'blur must remain enabled')
            assert mask.boundingRect() == QRect(1, 1, width - 2, height - 2), (path, mask.boundingRect())
            assert mask.rectCount() > 1, (path, 'rounded corners became a rectangle')
            for x, y in ((1, 1), (width - 2, 1), (1, height - 2), (width - 2, height - 2)):
                assert not mask.contains(QPoint(x, y)), (path, 'blur leaks into a rounded corner')
            assert mask.contains(QRect(16, 16, width - 32, height - 32)), (path, 'interior blur lost')
        for mode in ('light', 'dark'):
            config = (ROOT / f'themes/plasma/desktoptheme/sequoia-eleven-{mode}/plasmarc').read_text()
            assert '[BlurBehindEffect]\nenabled=true' in config
        print(f'PASS: {len(items)} rendered panel masks; inset edges, rounded corners, interior blur retained')
        app.exit(0)
    except Exception:
        import traceback
        traceback.print_exc()
        app.exit(1)


QTimer.singleShot(200, verify)
raise SystemExit(app.exec())
