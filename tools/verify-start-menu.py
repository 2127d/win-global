#!/usr/bin/env python3
"""Exercise the actual Start menu QML offscreen on a private bus and disposable XDG directories."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / 'widgets/plasmoids/org.sequoia.startmenu'


def child(stage):
    os.environ.update(QT_QPA_PLATFORM=os.environ.get('SEQUOIA_START_TEST_PLATFORM', 'offscreen'), QT_QUICK_BACKEND=os.environ.get('QT_QUICK_BACKEND', 'software'),
                      XDG_CONFIG_HOME=str(stage / 'config'), XDG_DATA_HOME=str(stage / 'data'),
                      XDG_CACHE_HOME=str(stage / 'cache'), XDG_RUNTIME_DIR=str(stage / 'run'),
                      XDG_STATE_HOME=str(stage / 'state'))
    for name in ('config', 'data/applications', 'cache', 'run', 'state'):
        (stage / name).mkdir(parents=True, exist_ok=True, mode=0o700)
    (stage / 'data/plasma').mkdir()
    (stage / 'data/plasma/desktoptheme').symlink_to(ROOT / 'themes/plasma/desktoptheme')
    (stage / 'config/plasmarc').write_text('[Theme]\nname=sequoia-eleven-light\n')
    marker = stage / 'launched'
    (stage / 'data/applications/sequoia-start-test.desktop').write_text(
        '[Desktop Entry]\nType=Application\nName=Sequoia Launch Test\nIcon=applications-development\n'
        f'Exec=/usr/bin/touch {marker}\nCategories=Utility;\n')
    subprocess.run(['kbuildsycoca6', '--noincremental'], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    from PySide6.QtCore import QObject, QPointF, QUrl, Qt, qInstallMessageHandler
    from PySide6.QtGui import QIcon
    from PySide6.QtWidgets import QApplication
    from PySide6.QtQml import QQmlComponent, QQmlEngine, QQmlExpression, QQmlPropertyMap
    from PySide6.QtQuick import QQuickView
    from PySide6.QtTest import QTest

    app = QApplication([])
    app.setDesktopFileName("org.kde.plasmashell")
    QIcon.setThemeName('breeze')
    errors = []

    def messages(kind, context, message):
        if str(PACKAGE) in message or 'ReferenceError' in message or 'TypeError' in message:
            errors.append(message)
            print(message, file=sys.stderr)

    cfg = QQmlPropertyMap()
    defaults = {}
    for entry in ET.parse(PACKAGE / 'contents/config/main.xml').findall('.//{*}entry'):
        value = entry.find('{*}default').text or ''
        kind = entry.get('type')
        value = int(value) if kind == 'Int' else value == 'true' if kind == 'Bool' else value.split(',') if kind == 'StringList' else value
        defaults[entry.get('name')] = value
        cfg.insert(entry.get('name'), value)

    wrapper = stage / 'Preview.qml'
    wrapper.write_text('''import QtQuick
import org.kde.ksvg as KSvg
import "''' + (PACKAGE / 'contents/ui').as_uri() + '''" as Start
KSvg.FrameSvgItem {
    imagePath: "dialogs/background"
    width: testConfiguration.menuWidth; height: testConfiguration.menuHeight
    Start.MenuBackend { id: dataBackend; configuration: testConfiguration }
    QtObject {
        id: rejectedLaunch
        function trigger(index, action, argument) { return false; }
    }
    Start.StartMenu {
        id: start; objectName: "menu"; anchors.fill: parent; anchors.margins: 12
        configuration: testConfiguration; backend: dataBackend
        onDismissRequested: opened = false
        Component.onCompleted: opened = true
    }
}''')
    view = QQuickView()
    view.rootContext().setContextProperty('testConfiguration', cfg)
    view.setSource(QUrl.fromLocalFile(str(wrapper)))
    assert view.status() == QQuickView.Ready, view.errors()
    # Install after compilation: PySide's Python handler can deadlock with the
    # QML loader thread while setSource holds the GIL (e.g. WheelHandler warnings).
    if os.environ['QT_QPA_PLATFORM'] != 'wayland':
        qInstallMessageHandler(messages)
    view.show()
    QTest.qWait(700)
    menu = view.rootObject().findChild(QObject, 'menu')

    def evaluate(source):
        expression = QQmlExpression(QQmlEngine.contextForObject(menu), menu, source)
        result, _ = expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        return result

    def settle():
        QTest.qWait(320)
        pages = view.rootObject().findChild(QObject, 'startPages')
        for _ in range(100):
            if not pages.property('busy'):
                break
            QTest.qWait(20)
        assert not pages.property('busy'), 'Page transition did not finish'

    assert evaluate('opened')
    assert evaluate('backend.apps.count') > 0
    assert evaluate('backend.categories.count') > 0
    initial_favorites = evaluate('backend.favorites')
    favorite_roles = {bytes(name).decode(): role for role, name in initial_favorites.roleNames().items()}
    for row in range(initial_favorites.rowCount()):
        favorite_id = initial_favorites.data(initial_favorites.index(row, 0), favorite_roles['favoriteId'])
        assert evaluate('backend.isPinned(' + json.dumps(favorite_id) + ')'), favorite_id
    print('PASS: menu loads, opens, and discovers installed apps and categories')

    # Pin order and removal use persisted desktop IDs, not transient row numbers.
    evaluate('backend.togglePin("sequoia-start-test.desktop")')
    assert 'sequoia-start-test.desktop' in cfg.value('favorites').toVariant()
    evaluate('backend.movePin("sequoia-start-test.desktop", -1)')
    assert cfg.value('favorites').toVariant()[-2] == 'sequoia-start-test.desktop'
    evaluate('backend.togglePin("sequoia-start-test.desktop")')
    assert 'sequoia-start-test.desktop' not in cfg.value('favorites').toVariant()
    print('PASS: pin, reorder, and unpin persist in configuration')

    for destination in ('pinned', 'recommended', 'apps', 'category'):
        if destination == 'category':
            evaluate('showCategory(backend.categories.modelForRow(0), backend.categories.labelForRow(0))')
        else:
            evaluate('navigate(' + json.dumps(destination) + ')')
        settle()
        assert evaluate('page') == destination
        assert view.rootObject().findChild(QObject, 'startPages').property('currentItem').property('destination') == destination
        evaluate('goBack()')
        settle()
        assert evaluate('page') == 'home'
    for mode in (0, 1, 2):
        cfg.insert('appView', mode)
        settle()
    print('PASS: all pages, Back transitions, Category/List/Grid views')

    search = view.rootObject().findChild(QObject, 'startSearch')
    search.setProperty('text', 'Sequoia Launch Test')
    QTest.qWait(1800)
    assert evaluate('backend.searchResults !== null && backend.searchResults.count > 0')
    QTest.keyClick(view, Qt.Key_Escape)
    settle()
    assert search.property('text') == '' and evaluate('opened')
    print('PASS: real application search and Escape clearing')

    evaluate('launch(rejectedLaunch, 0)')
    settle()
    assert evaluate('launchError.length > 0 && opened && !launching')
    evaluate('backend.togglePin("sequoia-start-test.desktop")')
    settle()
    favorites = evaluate('backend.favorites')
    roles = {bytes(name).decode(): role for role, name in favorites.roleNames().items()}
    row = next(row for row in range(favorites.rowCount()) if favorites.data(favorites.index(row, 0), roles['favoriteId']) == 'sequoia-start-test.desktop')
    evaluate(f'launch(backend.favorites, {row})')
    QTest.qWait(1200)
    assert marker.exists(), 'The fixture desktop entry was not launched'
    assert not evaluate('opened')
    evaluate('opened = true')
    settle()
    assert evaluate('page') == 'home' and evaluate('opened')
    cfg.insert('animationsEnabled', False)
    evaluate('navigate("apps")')
    QTest.qWait(30)
    assert evaluate('page') == 'apps'
    evaluate('opened = false')
    QTest.qWait(30)
    assert not evaluate('opened')
    evaluate('opened = true')
    QTest.qWait(30)
    assert evaluate('opened') and evaluate('page') == 'home'
    print('PASS: failed-launch feedback, real desktop launch, close/reopen, reduced motion')

    # Exercise the real native popup, including the background's window lifetime.
    popup_component = QQmlComponent(view.engine(), QUrl.fromLocalFile(str(PACKAGE / 'contents/ui/StartMenuPopup.qml')))
    popup = popup_component.createWithInitialProperties({'configuration': cfg, 'backend': evaluate('backend')})
    assert popup is not None, [e.toString() for e in popup_component.errors()]

    def popup_eval(source):
        expression = QQmlExpression(QQmlEngine.contextForObject(popup), popup, source)
        result, _ = expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        return result

    cfg.insert('animationsEnabled', True)
    cfg.insert('animationDuration', 600)
    popup.setProperty('opened', True)
    settle()
    popup_pages = popup.findChild(QObject, 'startPages')
    home = popup_pages.property('currentItem')
    for _ in range(12):
        popup.setProperty('opened', False)
        assert not popup.property('visible'), 'Native background must hide synchronously'
        assert not popup_eval('mainItem.opened')
        QTest.qWait(20)
        assert popup_pages.property('currentItem') == home, 'Closing home must not rebuild its delegates'
        popup.setProperty('opened', True)
        assert popup.property('visible') and popup_eval('mainItem.opened')
        QTest.qWait(20)
        assert popup_pages.property('currentItem') == home
    # Click the actual controls and verify separate themed popup windows.
    selector = popup.findChild(QObject, 'appViewSelector')
    view_menu = popup.findChild(QObject, 'startViewMenu')
    power_button = popup.findChild(QObject, 'startPowerButton')
    power_menu = popup.findChild(QObject, 'startPowerMenu')

    def click_control(button):
        QTest.mouseClick(popup, Qt.LeftButton, Qt.NoModifier,
                         button.mapToScene(QPointF(button.width() / 2, button.height() / 2)).toPoint())
        QTest.qWait(180)

    cfg.insert('appView', 0)
    click_control(selector)
    assert popup.property('visible') and view_menu.property('visible')
    point = selector.mapToGlobal(QPointF(0, selector.height()))
    assert abs(view_menu.x() - point.x()) <= 1, (view_menu.x(), point.x(), popup.x())
    assert abs(view_menu.y() - point.y() - 4) <= 1
    assert view_menu.width() == round(selector.width())
    assert view_menu.transientParent() == popup
    assert view_menu.grabWindow().save(str(ROOT / 'preview/start-view-menu.png'))
    QTest.keyClick(view_menu, Qt.Key_Down)
    QTest.keyClick(view_menu, Qt.Key_Return)
    QTest.qWait(180)
    assert cfg.value('appView') == 1 and popup.property('visible'), (cfg.value('appView'), popup.property('visible'))
    assert not view_menu.property('visible')
    click_control(selector)
    QTest.keyClick(view_menu, Qt.Key_Escape)
    QTest.qWait(100)
    assert not view_menu.property('visible') and popup.property('visible')
    click_control(power_button)
    assert power_menu.property('visible') and popup.property('visible')
    point = power_button.mapToGlobal(QPointF(power_button.width(), 0))
    assert abs(power_menu.x() + power_menu.width() - point.x()) <= 1
    assert abs(power_menu.y() + power_menu.height() + 4 - point.y()) <= 1
    assert power_menu.transientParent() == popup
    assert power_menu.grabWindow().save(str(ROOT / 'preview/start-power-menu.png'))
    QTest.keyClick(power_menu, Qt.Key_Escape)
    QTest.qWait(100)
    assert not power_menu.property('visible') and popup.property('visible')
    # Parent dismissal must also remove its transient menu.
    click_control(power_button)
    popup.setProperty('opened', False)
    assert not power_menu.property('visible') and not popup.property('visible')
    popup.setProperty('opened', True)
    cfg.insert('appView', 0)
    QTest.qWait(100)
    print('PASS: View opens below its control, Power opens above/right-aligned, keyboard selection/Escape and child dismissal')

    popup_eval('mainItem.navigate("apps")')
    QTest.qWait(650)
    popup.setProperty('opened', False)
    assert not popup.property('visible')
    QTest.qWait(40)
    assert popup_eval('mainItem.page') == 'home'
    # Interrupt an in-flight page transition and reopen before deferred cleanup.
    popup.setProperty('opened', True)
    popup_eval('mainItem.navigate("apps")')
    popup.setProperty('opened', False)
    popup.setProperty('opened', True)
    QTest.qWait(650)
    assert popup.property('visible') and popup_eval('mainItem.opened')
    popup_eval('mainItem.dismissRequested()')
    assert not popup.property('visible')
    QTest.qWait(40)
    assert popup_eval('mainItem.page') == 'home'
    cfg.insert('animationsEnabled', False)
    popup.setProperty('opened', True)
    popup_eval('mainItem.settingsRequested()')
    assert not popup.property('visible')
    popup.deleteLater()
    QTest.qWait(40)
    cfg.insert('animationDuration', defaults['animationDuration'])
    print('PASS: native popup hides immediately, warm home reuse, rapid toggles, navigation cleanup, settings dismissal')

    # Large collections must not construct tiles beyond their display limit.
    collection_component = QQmlComponent(view.engine())
    collection_component.setData(('import QtQuick\nimport "' + (PACKAGE / 'contents/ui').as_uri() +
        '" as Start\nStart.AppCollection { width: 600; limit: 4; sourceModel: ListModel { id: entries } '
        'Component.onCompleted: { for (let i = 0; i < 500; ++i) entries.append({display: "Fixture " + i}); } }').encode(),
        QUrl.fromLocalFile(str(stage / 'CollectionFixture.qml')))
    collection = collection_component.createWithInitialProperties({'menu': menu})
    assert collection is not None, [e.toString() for e in collection_component.errors()]
    collection.setParentItem(view.rootObject())

    def visual_items(item, name):
        return ([item] if item.objectName() == name else []) + [
            found for child in item.childItems() for found in visual_items(child, name)]

    QTest.qWait(40)
    assert len(visual_items(collection, 'startAppTile')) == 4
    collection.setProperty('limit', 8)
    QTest.qWait(30)
    assert len(visual_items(collection, 'startAppTile')) == 8
    collection.setProperty('populated', False)
    QTest.qWait(30)
    assert len(visual_items(collection, 'startAppTile')) == 0
    collection.deleteLater()
    cfg.insert('appView', 0)
    settle()
    assert len(visual_items(view.rootObject(), 'startAppTile')) <= (
        min(evaluate('backend.favorites.count'), defaults['pinnedRows'] * defaults['gridColumns']) +
        min(evaluate('backend.recent.count'), defaults['recommendationCount']))
    assert len(visual_items(view.rootObject(), 'startCategoryIcon')) <= 4 * evaluate('backend.categories.count')
    print('PASS: 500-row collection creates only requested tiles; hidden app views and category previews stay bounded')

    # Instantiate every settings page and exercise its configuration contract.
    for name in ('ConfigGeneral', 'ConfigPinned', 'ConfigAnimations'):
        component = QQmlComponent(view.engine(), QUrl.fromLocalFile(str(PACKAGE / 'contents/ui' / (name + '.qml'))))
        settings = component.create()
        assert settings is not None, [e.toString() for e in component.errors()]
        for key, value in defaults.items():
            if settings.metaObject().indexOfProperty('cfg_' + key) >= 0:
                assert settings.setProperty('cfg_' + key, value), key
                actual = settings.property('cfg_' + key)
                if hasattr(actual, 'toVariant'): actual = actual.toVariant()
                assert actual == value, (key, actual, value)
        settings.deleteLater()
        QTest.qWait(40)
    print('PASS: all three settings pages load and round-trip schema defaults')
    # Model a floating panel's painted surface and transparent margins. Move it
    # between screens and edges without involving the user's real desktop.
    panel_component = QQmlComponent(view.engine())
    panel_component.setData(('''import QtQuick
import org.kde.plasma.core as PlasmaCore
import "''' + (PACKAGE / 'contents/ui').as_uri() + '''" as Start
Window {
    id: panel
    x: 510; y: 1000; width: 900; height: 80
    property int thickness: 64
    property rect testScreen: Qt.rect(0, 0, 1920, 1080)
    property int edge: PlasmaCore.Types.BottomEdge
    Item {
        id: surface
        anchors.fill: parent
        property point floatingTranslucentItemOffset: Qt.point(8, 8)
        property int leftFloatingPadding: 8
        property int rightFloatingPadding: 8
        property int topFloatingPadding: 8
        property int bottomFloatingPadding: 8
    }
    Start.PopupPlacement {
        objectName: "placement"; anchor: surface
        screenRect: panel.testScreen; location: panel.edge
        requestedWidth: testConfiguration.menuWidth; requestedHeight: testConfiguration.menuHeight
    }
}''').encode(), QUrl.fromLocalFile(str(stage / 'PanelFixture.qml')))
    panel = panel_component.create()
    assert panel is not None, [e.toString() for e in panel_component.errors()]
    position = panel.findChild(QObject, 'placement')
    QTest.qWait(30)
    assert position.property('popupX') == (1920 - defaults['menuWidth']) // 2
    assert position.property('popupWidth') == defaults['menuWidth']
    assert position.property('popupHeight') == defaults['menuHeight']
    assert position.property('popupY') + position.property('popupHeight') == 1005

    def change_panel(source):
        expression = QQmlExpression(QQmlEngine.contextForObject(panel), panel, source)
        expression.evaluate()
        assert not expression.hasError(), expression.error().toString()
        QTest.qWait(30)

    change_panel('x = -1410; testScreen = Qt.rect(-1920, 0, 1920, 1080)')
    assert position.property('popupX') == -1920 + (1920 - defaults['menuWidth']) // 2
    change_panel('edge = PlasmaCore.Types.TopEdge; y = 0')
    assert position.property('popupY') == 75
    change_panel('edge = PlasmaCore.Types.BottomEdge; testScreen = Qt.rect(0, 0, 1280, 600); x = 190; y = 520')
    assert position.property('popupX') == (1280 - defaults['menuWidth']) // 2
    assert position.property('popupY') == 12
    assert position.property('popupHeight') == 513
    change_panel('testScreen = Qt.rect(0, 0, 600, 600)')
    assert position.property('popupWidth') == 576
    assert position.property('popupX') == 12
    assert position.property('popupHeight') == 513
    change_panel('testScreen = Qt.rect(0, 0, 1920, 1080); edge = PlasmaCore.Types.LeftEdge; x = 0; y = 90; width = 80; height = 900')
    assert position.property('popupX') == 75
    assert position.property('popupY') == (1080 - defaults['menuHeight']) // 2
    change_panel('edge = PlasmaCore.Types.RightEdge; x = 1840')
    assert position.property('popupX') + position.property('popupWidth') == 1845
    assert position.property('popupY') == (1080 - defaults['menuHeight']) // 2
    panel.deleteLater()
    QTest.qWait(30)
    print('PASS: configured menu size, screen centering, 3px floating-panel gap, all panel edges, second monitor, small-screen fit')
    cfg.insert('appView', 0)
    cfg.insert('animationsEnabled', True)
    cfg.insert('favorites', defaults['favorites'])
    settle()
    output = ROOT / 'preview/start-menu.png'
    assert view.grabWindow().save(str(output))
    assert not errors, '\n'.join(errors)
    print(f'PASS: no widget QML runtime errors; preview saved to {output}')
    view.close()
    qInstallMessageHandler(None)


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--child':
        child(Path(sys.argv[2]))
    else:
        with tempfile.TemporaryDirectory(prefix='sequoia-start-test-') as directory:
            stage = Path(directory)
            # Activated services inherit the bus daemon's environment, so
            # isolate their state before starting the bus, not just the client.
            env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
            for kind in ('CONFIG', 'DATA', 'CACHE', 'STATE'):
                path = stage / kind.lower()
                path.mkdir(mode=0o700)
                env[f'XDG_{kind}_HOME'] = str(path)
            (stage / 'run').mkdir(mode=0o700)
            env['XDG_RUNTIME_DIR'] = str(stage / 'run')
            bus = stage / 'bus.conf'
            bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir=' + directory + '</listen>'
                           '<policy context="default"><allow send_destination="*"/><allow eavesdrop="true"/>'
                           '<allow own="*"/></policy></busconfig>')
            subprocess.run(['dbus-run-session', '--config-file=' + str(bus), '--', sys.executable,
                            str(Path(__file__).resolve()), '--child', directory], env=env, check=True, timeout=60)
