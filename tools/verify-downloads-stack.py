#!/usr/bin/env python3
"""Check the real Downloads fan against temporary files without opening them."""
import os
from pathlib import Path
import sys
import tempfile
from preview_session import ensure_private_session
ensure_private_session()
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QUICK_CONTROLS_STYLE='Fusion')
from PySide6.QtCore import QUrl, Qt, QObject
from PySide6.QtGui import QFont, QIcon
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest
ROOT = Path(__file__).resolve().parents[1]
app = QApplication([])
app.setFont(QFont('Adwaita Sans', 10))
QIcon.setThemeSearchPaths([str(ROOT/'themes/icons'),'/usr/share/icons'])
QIcon.setThemeName('SequoiaTahoe-light')
view = QQuickView()
view.setResizeMode(QQuickView.SizeRootObjectToView)
view.setSource(QUrl.fromLocalFile(str(ROOT/'widgets/plasmoids/org.sequoia.downloads/contents/ui/FanView.qml')))
assert view.status() == QQuickView.Ready, view.errors()
root = view.rootObject()
with tempfile.TemporaryDirectory(prefix='downloads-fan-fixture-') as temporary:
    folder = Path(temporary)
    names = ['older notes.txt', 'photo.jpg', 'archive.zip', 'Report #1 & résumé.pdf', 'Long filename with a detailed project description for reviewing.txt', 'song.flac', 'clip.mp4', 'Another document.pdf', 'newest file.txt', 'design.pdf', 'recent.zip']
    for i, name in enumerate(names):
        file = folder/name
        file.touch()
        os.utime(file, (1700000000+i, 1700000000+i))
    (folder/'.hidden').touch()
    root.setProperty('folderUrl', QUrl.fromLocalFile(temporary))
    view.resize(320,536)
    view.show()
    QTest.qWait(600)
    assert root.property('totalCount') == 11
    def entry(i):
        pending=[root]
        while pending:
            item=pending.pop()
            if item.objectName() == 'downloadEntry'+str(i): return item
            pending.extend(item.childItems())
    assert entry(0).property('text') == 'recent.zip', entry(0).property('text')
    assert entry(9) and not entry(10)
    opened=[]; root.fileActivated.connect(lambda url: opened.append(url))
    entry(0).forceActiveFocus();QTest.keyClick(view, Qt.Key_Return)
    assert opened[-1].toLocalFile() == str(folder/names[-1])
    QTest.keyClick(view, Qt.Key_Up)
    assert entry(1).hasActiveFocus()
    # URL encoding and elided names still open the exact file.
    special=next(entry(i) for i in range(10) if '#' in entry(i).property('text'))
    special.forceActiveFocus();QTest.keyClick(view, Qt.Key_Space)
    assert opened[-1].toLocalFile() == str(folder/names[3])
    dismissed=[];root.dismissed.connect(lambda: dismissed.append(True))
    QTest.keyClick(view, Qt.Key_Escape);assert dismissed
    folderOpened=[];root.folderRequested.connect(lambda:folderOpened.append(True))
    button=root.findChild(QObject,'openDownloadsFolder');button.forceActiveFocus();QTest.keyClick(view,Qt.Key_Return)
    assert folderOpened
    new=folder/'just downloaded.pdf';new.touch();QTest.qWait(500)
    assert root.property('totalCount')==12 and entry(0).property('text')==new.name
    assert button.y() < entry(9).y() < entry(0).y()
    view.grabWindow().save(str(ROOT/'preview/downloads-fan.png'))
    folded=[];root.folded.connect(lambda:folded.append(True))
    startY=entry(9).y()
    root.setProperty('opened',False)
    duration=root.property('motionDuration')
    QTest.qWait(round(duration/2))
    assert 0 < root.property('reveal') < 1 and not folded
    assert entry(9).y() > startY and entry(9).property('foldScale') < 1
    QTest.qWait(duration+50)
    assert folded and root.property('reveal')==0
    root.setProperty('opened',True)
    duration=root.property('motionDuration')
    QTest.qWait(round(duration/2))
    assert 0 < root.property('reveal') < 1
    QTest.qWait(duration+50)
    assert root.property('reveal')==1 and entry(9).y()==startY
    root.setProperty('maximumHeight',300);QTest.qWait(100)
    assert root.property('capacity')==4 and entry(3) and not entry(4)
    for file in folder.iterdir(): file.unlink()
    QTest.qWait(500)
    assert root.property('totalCount')==0 and not entry(0)
print('PASS: newest-first fan, ten-file limit, encoded URLs, keyboard activation/navigation, Escape, folder action, live refresh, bidirectional travel/shrink animation, short-screen cap and empty state')
