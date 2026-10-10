#!/usr/bin/env python3
"""Check the contacts presentation with sample data, without accessing address books."""
import os
from pathlib import Path
import sys
import tempfile
from preview_session import ensure_private_session

ensure_private_session()

ROOT = Path(__file__).resolve().parents[1]
dark = '--dark' in sys.argv
large = '--large' in sys.argv
os.environ.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
if '--rhi' in sys.argv:
    os.environ.update(QT_QPA_PLATFORM='wayland', QT_QUICK_BACKEND='rhi')
from PySide6.QtCore import QObject, Slot, QAbstractListModel, QModelIndex, Qt, QUrl, qInstallMessageHandler, QPoint
from PySide6.QtGui import QPalette, QColor, QFont
from PySide6.QtWidgets import QApplication
from PySide6.QtQuick import QQuickView
from PySide6.QtTest import QTest

class Translations(QObject):
    @Slot(str, result=str)
    def i18n(self, text): return text
    @Slot(str, str, result=str)
    def i18nd(self, domain, text): return text
    @Slot(str, str, int, result=str)
    def i18np(self, one, many, count): return (one if count == 1 else many).replace('%1', str(count))

class Rows(QAbstractListModel):
    def __init__(self, rows):
        super().__init__()
        self.rows = rows
        self.filtered = rows
        self.names = list(rows[0]) if rows else []
    def rowCount(self, parent=QModelIndex()): return 0 if parent.isValid() else len(self.filtered)
    def roleNames(self): return {256 + i: name.encode() for i, name in enumerate(self.names)}
    def data(self, index, role=Qt.DisplayRole):
        return self.filtered[index.row()].get(self.names[role - 256]) if index.isValid() and 256 <= role < 256 + len(self.names) else None
    @Slot(str)
    def search(self, text):
        self.beginResetModel()
        self.filtered = [row for row in self.rows if text.casefold() in row['display'].casefold()]
        self.endResetModel()

errors = []
def message(kind, context, text):
    print(text, file=sys.stderr)
    if any(x in text for x in ('ReferenceError', 'TypeError', 'Binding loop', 'Unable to assign', 'Cannot assign', 'is not a type', 'unavailable')): errors.append(text)
qInstallMessageHandler(message)
with tempfile.TemporaryDirectory(prefix='sequoia-contacts-preview-') as temp:
    os.environ.update(XDG_CONFIG_HOME=temp, XDG_DATA_HOME=temp, XDG_STATE_HOME=temp+'/state', XDG_CACHE_HOME=temp+'/cache')
    variant = 'dark' if dark else 'light'
    Path(temp, 'kdeglobals').write_text((ROOT / f'color-schemes/SequoiaEleven{variant.title()}Solid.colors').read_text() + f'\n[Icons]\nTheme=SequoiaTahoe-{variant}\n')
    Path(temp, 'plasmarc').write_text(f'[Theme]\nname=sequoia-eleven-{variant}\n')
    Path(temp, 'plasma').symlink_to(ROOT / 'plasma')
    Path(temp, 'icons').symlink_to(ROOT / 'icons')
    app = QApplication([])
    palette = QPalette()
    for role in (QPalette.Window, QPalette.Base, QPalette.Button): palette.setColor(role, QColor('#252629' if dark else '#edf0f4'))
    for role in (QPalette.Text, QPalette.WindowText, QPalette.ButtonText): palette.setColor(role, QColor('#f2f3f5' if dark else '#252629'))
    palette.setColor(QPalette.Highlight, QColor('#8db3ff' if dark else '#2563db'))
    palette.setColor(QPalette.HighlightedText, QColor('#161a22' if dark else '#ffffff'))
    app.setPalette(palette)
    app.setFont(QFont('Adwaita Sans', 13 if large else 10))
    if '--rtl' in sys.argv: app.setLayoutDirection(Qt.RightToLeft)
    names = ['Alex Morgan', 'Avery Chen', 'Blair Anderson', 'Cameron Taylor', 'Casey Williams', 'Charlie Reed', 'Drew Bennett', 'Ellis Parker', 'Jordan Smith', 'Morgan Thompson']
    contacts = Rows([dict(display=name, itemId=i, addressee=None) for i, name in enumerate(names)])
    phones = Rows([dict(type='Mobile', display='+1 (555) 010-2048'), dict(type='Work', display='+1 (555) 010-8096')])
    emails = Rows([dict(type='Personal', display='alex.morgan@example.com'), dict(type='Work', display='alexandria.morgan.department@example.com')])
    addresses = Rows([dict(typeLabel='Home', formattedAddress='124 Willow Avenue\nApartment 302\nPortland, Oregon 97205\nUnited States')])
    translations = Translations()
    view = QQuickView()
    view.rootContext().setContextObject(translations)
    for name, value in dict(testContacts=contacts, testPhones=phones, testEmails=emails, testAddresses=addresses).items(): view.rootContext().setContextProperty(name, value)
    source = ROOT / '.staging/control-center-data/sequoia-eleven/control-center/contacts'
    qml = Path(temp, 'Preview.qml')
    qml.write_text('''import QtQuick
import QtQuick.Controls as QQC2
import "''' + source.as_uri() + '''" as Custom
Rectangle {
    id: root
    width: 432; height: 472
    color: "''' + ('#252629' if dark else '#edf0f4') + '''"
    property int opened: -1
    property string activated: ""
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    function showDetails() { stack.push(details); }
    function goBack() { stack.pop(); }
    QQC2.StackView {
        id: stack
        anchors.fill: parent; anchors.margins: 8
        initialItem: Custom.ContactsView {
            contactModel: testContacts
            onSearchChanged: query => testContacts.search(query)
            onOpenContact: itemId => { root.opened = itemId; root.showDetails(); }
        }
    }
    Component {
        id: details
        Custom.ContactDetails {
            objectName: "contactProfile"
            contact: ({formattedName: "Alexandria Morgan", nickName: "Alex", photoUrl: "", preferredEmail: "alex.morgan@example.com",
                phoneModel: testPhones, emailModel: testEmails, addressesModel: testAddresses, birthday: new Date(1990, 5, 14)})
            onActivate: (scheme, value) => root.activated = scheme + value
            onShowCode: stack.push(code)
        }
    }
    Component {
        id: code
        Custom.ContactCode {
            objectName: "contactCode"
            qrCodeData: "BEGIN:VCARD\\nVERSION:3.0\\nFN:Alex Morgan\\nEND:VCARD"
            onTypeSelected: value => barcodeType = value
        }
    }
}''')
    view.setSource(QUrl.fromLocalFile(str(qml)))
    assert view.status() == QQuickView.Ready, view.errors()
    view.show(); QTest.qWait(650)
    def items(item):
        result = [item]
        for child in item.childItems(): result.extend(items(child))
        return result
    def find(name): return next(i for i in items(view.rootObject()) if i.objectName() == name)
    suffix = variant + ('-large' if large else '') + ('-rtl' if '--rtl' in sys.argv else '')
    def capture(name):
        assert view.grabWindow().save(str(ROOT / f'preview/contacts-{suffix}-{name}.png'))
    assert find('contactsList').property('count') == len(names)
    capture('list')
    search = find('contactsSearch')
    search.setProperty('text', 'zzzz'); QTest.qWait(100)
    assert find('contactsList').property('count') == 0
    capture('empty-search')
    search.setProperty('text', 'alex'); QTest.qWait(100)
    assert find('contactsList').property('count') == 1
    search.forceActiveFocus()
    QTest.keyClick(view, Qt.Key_Down); QTest.keyClick(view, Qt.Key_Return); QTest.qWait(650)
    assert view.rootObject().property('opened') == 0
    capture('profile')
    profile = find('contactProfile')
    if '--debug' in sys.argv:
        for item in items(profile):
            if item.metaObject().indexOfProperty('isMask') >= 0:
                print('Icon:', item.property('source'), item.property('color'), item.property('enabled'), flush=True)
    assert find('contactPhones').property('count') == 2
    assert find('contactEmails').property('count') == 2
    assert find('contactAddresses').property('count') == 1
    # Long fields remain inside their cards; check all text labels, including offscreen content.
    for item in items(profile):
        if item.metaObject().indexOfProperty('wrapMode') >= 0 and item.property('text'):
            assert item.width() >= 0
            assert item.property('contentWidth') <= item.width() + 1, (item.property('text'), item.width(), item.property('contentWidth'))
    button = find('contactShare')
    point = button.mapToScene(QPoint(int(button.width()/2), int(button.height()/2)))
    QTest.mouseClick(view, Qt.LeftButton, pos=point.toPoint()); QTest.qWait(650)
    assert find('contactCode').isVisible()
    capture('code')
    view.rootObject().goBack(); QTest.qWait(650)
    view.rootObject().goBack(); QTest.qWait(650)
    assert find('contactsSearch').property('text') == 'alex'
    search = find('contactsSearch'); search.setProperty('text', ''); QTest.qWait(100)
    assert find('contactsList').property('count') == len(names)
    assert not errors, '\n'.join(errors)
    print('PASS: contacts list, filtering, keyboard open, profile fields, QR sharing and Back preserve search (' + suffix + ')', flush=True)
    view.setSource(QUrl()); QTest.qWait(50)
# Some KDE QML plugins retain font resources through process teardown.
os._exit(0)
