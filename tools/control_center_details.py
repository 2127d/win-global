# SPDX-License-Identifier: MIT
"""Apply presentation-only changes to the installed KDE detail applets."""
import re
import shutil
from pathlib import Path

APPLETS = ('volume', 'brightness', 'battery', 'bluetooth', 'networkmanagement',
           'notifications', 'clipboard', 'devicenotifier', 'printmanager', 'cameraindicator')


def style_details(destination, source, plugin_dir, extract, replace):
    shared = destination / 'details'
    shutil.copytree(source / 'details', shared)
    dependencies = []
    for name in APPLETS:
        plugin = plugin_dir / f'org.kde.plasma.{name}.so'
        if not plugin.exists():
            continue
        directory = destination / name
        extract(plugin, f':/qt/qml/plasma/applet/org/kde/plasma/{name}/', directory)
        dependencies.append(plugin)
        for path in directory.rglob('*.qml'):
            text = path.read_text()
            original = text
            # Preserve compiled backend types, but resolve sibling views locally.
            if name == 'devicenotifier':
                text = text.replace('import plasma.applet.org.kde.plasma.devicenotifier\n',
                                    'import plasma.applet.org.kde.plasma.devicenotifier as NativeDevices\n')
                text = re.sub(r'\b(DeviceFilterControl|StateInfo)\b', r'NativeDevices.\1', text)
            replacements = {
                'PlasmaExtras.Representation {': 'Details.DetailRepresentation {',
                'PlasmaExtras.PlasmoidHeading {': 'Details.DetailHeading {',
                'PlasmaExtras.ExpandableListItem {': 'Details.DetailExpandable {',
                'PlasmaComponents3.ItemDelegate {': 'Details.DetailDelegate {',
                'PC3.ItemDelegate {': 'Details.DetailDelegate {',
                'PC3.TabButton {': 'Details.DetailTabButton {',
                'PlasmaComponents3.TabButton {': 'Details.DetailTabButton {',
                'PC3.TabBar {': 'Details.DetailTabBar {',
                'PlasmaComponents3.TabBar {': 'Details.DetailTabBar {',
            }
            # Apply shared detail styling here. Notification banners and their
            # shared components are refined separately in style_notifications.
            if path.parent == directory:
                for old, new in replacements.items():
                    text = text.replace(old, new)
                # Rounded delegates paint hover, expansion and keyboard focus.
                # A second native SVG highlight leaks outside their corners.
                list_highlights = {
                    ('bluetooth', 'FullRepresentation.qml'): 'highlight: PlasmaExtras.Highlight {}',
                    ('networkmanagement', 'ConnectionListPage.qml'): 'highlight: PlasmaExtras.Highlight { }',
                    ('printmanager', 'FullRepresentation.qml'): 'highlight: PlasmaExtras.Highlight {}',
                    ('devicenotifier', 'FullRepresentation.qml'): 'highlight: PlasmaExtras.Highlight\n            {\n            }',
                }
                highlight = list_highlights.get((name, path.name))
                if highlight:
                    text = replace(text, highlight, 'highlight: null // The rounded delegate owns its highlight.')
                if name == 'cameraindicator' and path.name == 'FullRepresentation.qml':
                    text = replace(text, 'PlasmaComponents.Page {', 'Details.DetailRepresentation {')
                if name == 'brightness' and path.name == 'PopupDialog.qml':
                    # Repeater.count includes displays whose delegates have not
                    # been constructed yet. Link each neighbor when it exists.
                    for neighbor, prop, value in (
                        ('index - 1', 'nextSlider', 'item'),
                        ('index + 1', 'previousSlider', 'item'),
                        ('index - 1', 'nextSlider', 'itemAt(index)'),
                        ('index + 1', 'previousSlider', 'itemAt(index)'),
                    ):
                        assignment = f'itemAt({neighbor}).{prop} = {value};'
                        text = replace(text, assignment, f'if (itemAt({neighbor})) {assignment}')
                if 'Details.DetailDelegate {' in text:
                    text = replace(text, '    background.visible: highlighted', '    background.visible: true')
                if name == 'volume' and path.name == 'main.qml':
                    text = replace(text, '            rightPadding: -1', '            rightPadding: 10')
                    text = replace(text, '            bottomPadding: -bottomInset', '            bottomPadding: 8')
                    for section in ('upperSection', 'lowerSection'):
                        text = replace(text, f'                        id: {section}\n',
                                       f'                        id: {section}\n                        spacing: 8\n')
                        text = replace(text, f'''highlight: PlasmaExtras.Highlight {{
                            visible: {section}.activeFocus
                        }}''', 'highlight: null // DetailDelegate draws keyboard focus inside its rounded surface.')
                if name == 'notifications' and path.name == 'DraggableDelegate.qml':
                    text = replace(text, '    id: swipeDelegate', '''    id: swipeDelegate
    topPadding: 8
    bottomPadding: 8
    background: Details.DetailSurface {
        objectName: "sequoiaNotificationCard"
        focused: swipeDelegate.activeFocus
        hovered: swipeDelegate.hovered
    }''')
                    text = replace(text, 'implicitHeight: contentItem.implicitHeight',
                                   'implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding')
                if 'Details.' in text:
                    text = text.replace('import QtQuick\n', 'import QtQuick\nimport "../details" as Details\n', 1)
            if text != original:
                path.write_text(text)
    # Klipper ships its views outside the applet plugin. Copy just those views;
    # the native history model, actions, editing and barcode code remain intact.
    from PySide6.QtCore import QLibraryInfo
    clipboard_source = Path(QLibraryInfo.path(QLibraryInfo.QmlImportsPath)) / 'org/kde/plasma/private/clipboard'
    clipboard = destination / 'clipboard' / 'views'
    clipboard.mkdir()
    for path in clipboard_source.glob('*.qml'):
        dependencies.append(path)
        text = path.read_text()
        text = text.replace('import QtQuick\n', 'import QtQuick\nimport "../../details" as Details\n', 1)
        text = text.replace(': PlasmaExtras.PlasmoidHeading {', ': Details.DetailHeading {')
        text = text.replace('PlasmaComponents3.TabButton {', 'Details.DetailTabButton {')
        text = text.replace('PlasmaComponents3.TabBar {', 'Details.DetailTabBar {')
        if path.name == 'ClipboardItemDelegate.qml':
            text = replace(text, '''    background: PlasmaExtras.Highlight {
        anchors.fill: parent
        hovered: menuItem.ListView.isCurrentItem
    }''', '''    background: Details.DetailSurface {
        objectName: "sequoiaClipboardCard"
        radius: 14
        emphasized: menuItem.ListView.isCurrentItem
        focused: menuItem.activeFocus
        hovered: menuItem.hovered
    }''')
        if path.name == 'ClipboardMenu.qml':
            text = replace(text, 'bottomPadding: tabBar.visible ? 0 : metricsPlasmoidHeading.bottomPadding',
                           'bottomPadding: 8')
        (clipboard / path.name).write_text(text)
    dependencies.append(clipboard_source / 'libklipperplugin.so')
    path = destination / 'clipboard' / 'main.qml'
    text = replace(path.read_text(), 'import QtQuick\n', 'import QtQuick\nimport "views" as ClipboardViews\n')
    text = text.replace('Private.ClipboardMenu', 'ClipboardViews.ClipboardMenu')
    text = text.replace('Private.BarcodePage', 'ClipboardViews.BarcodePage')
    path.write_text(text)
    return dependencies
