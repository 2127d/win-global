#!/usr/bin/env python3
"""Build a version-checked local overlay of the installed KDE QML components."""
import hashlib
import json
import re
from pathlib import Path
import shutil
import tempfile
from PySide6.QtCore import QLibraryInfo

ROOT = Path(__file__).resolve().parents[1]
QML = Path(QLibraryInfo.path(QLibraryInfo.QmlImportsPath))
MODULES = ('org/kde/kirigami', 'org/kde/desktop', 'org/kde/ksysguard/page', 'org/kde/kcmutils', 'org/kde/newstuff')


def replace(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f'Installed KDE component changed; expected one occurrence of {old!r}')
    return text.replace(old, new, 1)


def background(text, new, *, inherited_root=None):
    """Replace a root background, or override a known inherited background."""
    # Ignore examples in comments and braces in strings. Only root bindings
    # count: a child's background must never be replaced by the page canvas.
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/|\bbackground\s*:|[{}]', re.S)
    depth = 0
    binding = None
    for token in tokens.finditer(text):
        value = token.group()
        if value == '{':
            depth += 1
        elif value == '}':
            depth -= 1
            if binding is not None and depth == 1:
                return text[:binding] + new + text[token.end():]
        elif depth == 1 and value.startswith('background'):
            if not re.match(r'\s*[\w.]+\s*\{', text[token.end():]):
                raise RuntimeError('Installed KDE component changed; expected an object background')
            start = text.rfind('\n', 0, token.start()) + 1
            binding = start if not text[start:token.start()].strip() else token.start()
    if binding is not None:
        raise RuntimeError('Unbalanced background in installed KDE component')
    if inherited_root is not None:
        # Some Kirigami Pages inherit their background from Qt Quick Controls.
        # Add the override only for the known root type and id used by our QML.
        anchor = re.compile(r'^' + re.escape(inherited_root)
                            + r'[ \t]*\{\s*id:[ \t]*root[ \t]*;?[ \t]*(?=\n)', re.M)
        matches = list(anchor.finditer(text))
        if len(matches) == 1:
            end = matches[0].end()
            return text[:end] + '\n\n' + new + text[end:]
    raise RuntimeError('Installed KDE component changed; no supported root background found')


def _build(destination):
    destination = Path(destination)
    if destination.exists():
        if not (destination / 'manifest.json').exists():
            raise RuntimeError(f'Refusing to replace unmanaged directory: {destination}')
        shutil.rmtree(destination)
    destination.mkdir(parents=True)
    fingerprints = {}
    for module in MODULES:
        source = QML / module
        if not source.is_dir():
            raise RuntimeError(f'Required QML module is missing: {source}')
        for path in source.rglob('*'):
            if not path.is_file():
                continue
            target = destination / path.relative_to(QML)
            target.parent.mkdir(parents=True, exist_ok=True)
            # Use the current system plugin; never ship copied KDE binaries.
            if '.so' in path.name:
                target.symlink_to(path)
            else:
                shutil.copy2(path, target)
            if path.suffix == '.qml' or path.name == 'qmldir' or '.so' in path.name:
                fingerprints[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
            if path.name == 'qmldir':
                target.write_text(''.join(line for line in path.read_text().splitlines(True)
                                          if not line.startswith('prefer ')))
    # Share the native Switch and SwitchDelegate logic, replacing only their
    # visual indicator. The system source remains fingerprinted for upgrades.
    switch_indicator = destination / 'org/kde/desktop/private/SwitchIndicator.qml'
    source = switch_indicator.read_text()
    if 'property T.AbstractButton control' not in source or 'property alias handle:' not in source:
        raise RuntimeError('Installed KDE switch indicator changed; review its interface before installing.')
    shutil.copy2(ROOT / 'themes/application-style/quick/SwitchIndicator.qml', switch_indicator)
    from menu_toggles import desktop_menu
    menu = destination / 'org/kde/desktop/MenuItem.qml'
    menu.write_text(desktop_menu(menu.read_text(), replace))
    shared = destination / 'org/sequoia/eleven'
    shared.mkdir(parents=True)
    for component in ('Island', 'Canvas', 'SidebarIcon', 'SidebarHighlight'):
        shutil.copy2(ROOT / f'themes/application-style/quick/{component}.qml', shared / f'{component}.qml')
    (shared / 'qmldir').write_text('module org.sequoia.eleven\nIsland 1.0 Island.qml\nCanvas 1.0 Canvas.qml\nSidebarIcon 1.0 SidebarIcon.qml\nSidebarHighlight 1.0 SidebarHighlight.qml\n')

    def patch(relative, transform):
        path = destination / relative
        try:
            text = transform(path.read_text())
        except (RuntimeError, ValueError) as error:
            raise RuntimeError(f'Cannot style installed KDE component {QML / relative}: {error}') from error
        path.write_text(text)

    import_island = 'import org.sequoia.eleven as Sequoia\n'
    base = 'org/kde/kirigami/'
    # Paint glass as a scene item, not the QQuickWindow clear color. This Qt/RHI
    # path presents a translucent clear color too brightly on Wayland, although
    # grabWindow() reports the expected RGBA. A Rectangle composites correctly
    # with the native titlebar, and nested pages still leave this one tint clear.
    patch(base + 'controls/AbstractApplicationWindow.qml', lambda t:
          replace(t, '    color: Platform.Theme.backgroundColor', '''    color: Platform.Theme.backgroundColor.a < 1 ? "transparent" : Platform.Theme.backgroundColor
    background: Rectangle {
        objectName: "sequoiaWindowTint"
        color: Platform.Theme.backgroundColor
    }'''))
    # Embedded ApplicationItems use their native QWidget host's tint instead.
    patch(base + 'controls/Page.qml', lambda t:
          background(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                     '''    background: Sequoia.Canvas {
        // The sidebar host paints one continuous island behind both its header
        // and navigation pages, including category transitions.
        visible: !/^(CategoriesPage|SubCategoryPage)_/.test(root.toString())
        solidColor: root.Platform.Theme.backgroundColor
    }''', inherited_root='QQC2.Page'))
    patch(base + 'controls/ApplicationItem.qml', lambda t:
          replace(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                  'background: Rectangle {\n            color: root.color\n        }',
                  '''background: Item {
            id: sequoiaApplicationBackground
            readonly property bool sidebar: /^SideBarItem_/.test(root.toString())
            Rectangle {
                objectName: "sequoiaSettingsSidebarCanvas"
                anchors.fill: parent
                visible: sequoiaApplicationBackground.sidebar
                Platform.Theme.colorSet: Platform.Theme.Window
                Platform.Theme.inherit: false
                // Preserve the window tint's alpha so compositor blur remains
                // visible around the floating sidebar, just like the content.
                color: Platform.Theme.backgroundColor
            }
            Sequoia.Canvas {
                anchors.fill: parent
                visible: !sequoiaApplicationBackground.sidebar
                solidColor: root.color
            }
            Sequoia.Island {
                objectName: "sequoiaSettingsSidebar"
                anchors.fill: parent
                visible: sequoiaApplicationBackground.sidebar
                Platform.Theme.colorSet: Platform.Theme.Window
                Platform.Theme.inherit: false
            }
        }'''))
    patch('org/kde/desktop/Page.qml', lambda t:
          background(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                     '    background: Sequoia.Canvas { solidColor: controlRoot.Kirigami.Theme.backgroundColor }'))
    # Unframed KCMs add a second, opaque View rectangle on top of Page.
    patch('org/kde/kcmutils/AbstractKCM.qml', lambda t:
          replace(replace(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                  '    Rectangle {\n        anchors.fill: parent\n        visible: !root.framedView',
                  '    Sequoia.Canvas {\n        anchors.fill: parent\n        visible: !root.framedView'),
                  '        Kirigami.Theme.colorSet: Kirigami.Theme.View\n        Kirigami.Theme.inherit: false\n        color: Kirigami.Theme.backgroundColor',
                  '        solidColor: Kirigami.Theme.backgroundColor'))
    patch(base + 'controls/OverlayDrawer.qml', lambda t:
          background(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                     '''    padding: 12
    background: Sequoia.Island {
        color: Platform.Theme.backgroundColor
        radius: parent?.objectName === "DiscoverDrawer" ? 22 : 14
    }'''))
    def global_drawer(t):
        # Keep Discover's category viewport as wide as its search field even
        # when the list overflows. Wheel, touch and keyboard scrolling remain.
        t = replace(t, '            id: scrollView', '''            id: scrollView
            QQC2.ScrollBar.vertical.policy: (QQC2.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false)
                ? QQC2.ScrollBar.AlwaysOff : QQC2.ScrollBar.AsNeeded''')
        t = replace(t, '                id: backItem', '''                id: backItem
                readonly property bool sequoiaDiscoverSidebar: QQC2.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false''')
        # Discover resets the drawer padding to zero. Give its whole layout
        # one inset so the header, scrolling list and progress footer all fit.
        t = replace(t, '        id: mainLayout', '''        id: mainLayout
        readonly property int sequoiaDrawerInset: parent?.objectName === "DiscoverDrawer" ? 12 : 0''')
        for edge in ('top', 'bottom', 'left', 'right'):
            margin = f'{edge}Margin: '
            if edge == 'top':
                margin += '(root.collapsed && !root.showHeaderWhenCollapsed ? -contentItem.y : 0) + '
            anchor = f'            {margin}root.{edge}Padding'
            t = replace(t, anchor, anchor + ' + mainLayout.sequoiaDrawerInset')
        return replace(replace(replace(replace(t,
              'leftPadding: root.edge === Qt.LeftEdge ? parent.SafeArea.margins.left : 0',
              'leftPadding: 12 + (root.edge === Qt.LeftEdge ? parent.SafeArea.margins.left : 0)'),
              'rightPadding: root.edge === Qt.RightEdge ? parent.SafeArea.margins.right : 0',
              'rightPadding: 12 + (root.edge === Qt.RightEdge ? parent.SafeArea.margins.right : 0)'),
              'topPadding: parent.SafeArea.margins.top', 'topPadding: 12 + parent.SafeArea.margins.top'),
              'bottomPadding: parent.SafeArea.margins.bottom', 'bottomPadding: 12 + parent.SafeArea.margins.bottom')
    patch(base + 'controls/GlobalDrawer.qml', global_drawer)
    def search_icon_spacing(t):
        t = replace(t, '    id: root', '''    id: root
    property real sequoiaIconInset: Platform.Units.smallSpacing * 2
    property int sequoiaIconSize: Platform.Units.iconSizes.sizeForLabels''')
        t = replace(t, 'anchors.leftMargin: Platform.Units.smallSpacing * 2',
                    'anchors.leftMargin: root.sequoiaIconInset')
        t = replace(t, 'implicitHeight: Platform.Units.iconSizes.sizeForLabels',
                    'implicitHeight: root.sequoiaIconSize')
        return replace(t, 'implicitWidth: Platform.Units.iconSizes.sizeForLabels',
                       'implicitWidth: root.sequoiaIconSize')
    patch(base + 'controls/SearchField.qml', search_icon_spacing)
    def discover_sidebar_item(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\n' + import_island)
        t = replace(t, '    id: listItem', '''    id: listItem

    readonly property bool sequoiaDiscoverSidebar: QQC2.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false''')
        t = replace(t, 'color: (listItem.highlighted || listItem.checked || listItem.down) ? Platform.Theme.highlightedTextColor : Platform.Theme.textColor',
                    '''color: listItem.sequoiaDiscoverSidebar
                ? ((listItem.highlighted || listItem.checked) ? Platform.Theme.linkColor : Platform.Theme.textColor)
                : ((listItem.highlighted || listItem.checked || listItem.down) ? Platform.Theme.highlightedTextColor : Platform.Theme.textColor)''')
        return replace(t, '''        Primitives.Icon {
            id: iconItem
            color: listItem.tAction.icon.color
            source: listItem.tAction.icon.name || listItem.tAction.icon.source

            readonly property int size: Platform.Units.iconSizes.smallMedium
            Layout.minimumHeight: size
            Layout.maximumHeight: size
            Layout.minimumWidth: size
            Layout.maximumWidth: size

            selected: (listItem.highlighted || listItem.checked || listItem.down)
            visible: source !== undefined && !listItem.isSeparator
        }''', '''        Item {
            id: iconItem
            readonly property var source: listItem.tAction.icon.name || listItem.tAction.icon.source
            readonly property int size: listItem.sequoiaDiscoverSidebar ? 26 : Platform.Units.iconSizes.smallMedium
            Layout.minimumHeight: size
            Layout.maximumHeight: size
            Layout.minimumWidth: size
            Layout.maximumWidth: size
            visible: source !== undefined && !listItem.isSeparator

            Primitives.Icon {
                anchors.fill: parent
                visible: !listItem.sequoiaDiscoverSidebar
                color: listItem.tAction.icon.color
                source: iconItem.source
                selected: listItem.highlighted || listItem.checked || listItem.down
            }
            Loader {
                anchors.fill: parent
                active: listItem.sequoiaDiscoverSidebar
                sourceComponent: Sequoia.SidebarIcon {
                    source: iconItem.source
                    selected: listItem.checked || listItem.highlighted
                    hovered: listItem.hovered
                }
            }
        }''')
    patch(base + 'controls/private/GlobalDrawerActionItem.qml', discover_sidebar_item)
    def sidebar_row_size(t):
        # Match the search field's 40px surface, with a 6px gap above each row.
        # Keep these metrics scoped to Discover's explicitly marked delegates.
        t = replace(t, '    id: controlRoot', '''    id: controlRoot
    readonly property bool sequoiaRoomySidebar: controlRoot.sequoiaDiscoverSidebar ?? false
    topPadding: sequoiaRoomySidebar ? 13 : padding''')
        t = replace(t, 'implicitHeight: Math.max(implicitBackgroundHeight',
                    'implicitHeight: Math.max(sequoiaRoomySidebar ? 46 : 0, implicitBackgroundHeight')
        t = replace(t, 'bottomPadding: typeof index',
                    'bottomPadding: sequoiaRoomySidebar ? 7 : typeof index')
        t = replace(t, 'topInset: TableView.view',
                    'topInset: sequoiaRoomySidebar ? 6 : TableView.view')
        return replace(t, 'bottomInset: TableView.view',
                       'bottomInset: sequoiaRoomySidebar ? 0 : TableView.view')
    patch('org/kde/desktop/ItemDelegate.qml', sidebar_row_size)
    def sidebar_background(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\n' + import_island)
        t = replace(t, '    id: background', '''    id: background
    readonly property bool sequoiaSidebar: control?.sequoiaDiscoverSidebar ?? false
    Loader {
        anchors.fill: parent
        anchors.leftMargin: -background.control.leftInset
        anchors.rightMargin: -background.control.rightInset
        active: background.sequoiaSidebar
        sourceComponent: Sequoia.SidebarHighlight { control: background.control }
    }''')
        if t.count('visible: background.control.enabled') != 2:
            raise RuntimeError('Sidebar background visibility changed; review before refreshing.')
        return t.replace('visible: background.control.enabled',
                         'visible: background.control.enabled && !background.sequoiaSidebar')
    patch('org/kde/desktop/private/DefaultListItemBackground.qml', sidebar_background)
    for relative, theme in [('controls/AbstractApplicationHeader.qml', 'Platform'),
                            ('styles/org.kde.desktop/AbstractApplicationHeader.qml', 'Kirigami')]:
        patch(base + relative, lambda t, theme=theme:
              background(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                         f'''    background: Sequoia.Island {{
        visible: root.parent?.parent?.objectName !== "DiscoverDrawer"
        color: {theme}.Theme.backgroundColor
    }}'''))
    def discover_header(t):
        t = replace(t, '    id: root', '''    id: root

    readonly property bool sequoiaDiscoverHeader: QQC.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false
    background.visible: !sequoiaDiscoverHeader''')
        t = replace(t, 'separatorVisible: pageRow ? pageRow.globalToolBar.separatorVisible : true',
                    'separatorVisible: !sequoiaDiscoverHeader && (pageRow ? pageRow.globalToolBar.separatorVisible : true)')
        return replace(t, '            Layout.leftMargin: {', '''            Layout.leftMargin: {
                if (root.sequoiaDiscoverHeader) {
                    const inset = root.page?.sequoiaTitleInset ?? root.page?.leftPadding ?? Platform.Units.gridUnit;
                    const navigationWidth = (navButtons.visible ? navButtons.width + layout.spacing : 0)
                        + (leadingHandle.visible ? leadingHandle.width + layout.spacing : 0);
                    return Math.max(0, inset - root.leftPadding - navigationWidth);
                }''')
    patch(base + 'controls/private/globaltoolbar/ToolBarPageHeader.qml', discover_header)

    def discover_title(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport QtQuick.Controls as QQC\n')
        return replace(t, '        id: heading', '''        id: heading

        Binding {
            target: heading
            property: "font.family"
            value: "Noto Sans"
            when: heading.QQC.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false
        }
        Binding {
            target: heading
            property: "font.weight"
            value: Font.Bold
            when: heading.QQC.ApplicationWindow.window?.sequoiaDiscoverHeaders ?? false
        }''')
    patch(base + 'controls/private/DefaultPageTitleDelegate.qml', discover_title)
    patch(base + 'controls/private/DefaultCardBackground.qml', lambda t:
          replace(replace(replace(replace(t, 'radius: Platform.Units.cornerRadius', 'radius: 14'),
                  'color: Qt.rgba(0, 0, 0, 0.05)', 'color: Qt.rgba(0, 0, 0, 0.12)'),
                  'property int borderWidth: 1',
                  'property int borderWidth: Platform.Theme.frameContrast > 0 ? 1 : 0'),
                  'property color defaultColor: Platform.Theme.backgroundColor',
                  'property color defaultColor: Qt.rgba(Platform.Theme.backgroundColor.r, Platform.Theme.backgroundColor.g, Platform.Theme.backgroundColor.b, 1)'))
    patch('org/kde/desktop/ToolBar.qml', lambda t:
          background(replace(t.replace('import QtQuick\n', 'import QtQuick\n' + import_island),
                  'padding: Kirigami.Units.smallSpacing', 'padding: 12'),
                  '    background: Sequoia.Island { implicitHeight: 44 }'))
    def drawer(t):
        t = t.replace('import QtQuick\n', 'import QtQuick\n' + import_island)
        return background(t, '    background: Sequoia.Island {}')
    patch('org/kde/desktop/Drawer.qml', drawer)
    def scroll_view(t):
        t = t.replace('import QtQuick\n', 'import QtQuick\n' + import_island)
        # Discover supplies its own edge-to-edge drawer layout, bypassing the
        # GlobalDrawer padding above. Inset its navigation viewport (including
        # scrollbars) without painting a second island inside the drawer.
        t = replace(t, '    id: controlRoot', '''    id: controlRoot

    readonly property bool sequoiaDiscoverDrawer: parent?.parent?.objectName === "DiscoverDrawer"
    readonly property bool sequoiaNavigationPage: /^(CategoriesPage|SubCategoryPage)_/.test(parent?.toString() ?? "")
    readonly property bool sequoiaInsetContent: background?.visible || sequoiaNavigationPage
        || sequoiaDiscoverDrawer''')
        t = t.replace('controlRoot.background?.visible', 'controlRoot.sequoiaInsetContent')
        # Padding positions the viewport but does not clip app-provided
        # ListViews/Flickables. Clip their contents at that inset rectangle so
        # partially visible rows cannot paint across the floating surface edge.
        t = replace(t, '    data: [', '''    data: [
        Binding {
            target: controlRoot.contentItem
            property: "clip"
            value: true
            when: controlRoot.sequoiaInsetContent
            restoreMode: Binding.RestoreBindingOrValue
        },''')
        return background(t, '''    background: Sequoia.Island {
        // The navigation host supplies its full-height background, like Discover.
        visible: controlRoot.Kirigami.StyleHints.showFramedBackground && !controlRoot.sequoiaNavigationPage && !controlRoot.sequoiaDiscoverDrawer
        readonly property real topPadding: controlRoot.sequoiaDiscoverDrawer ? 4 : 12
        readonly property real bottomPadding: controlRoot.sequoiaDiscoverDrawer ? 4 : 12
        readonly property real leftPadding: controlRoot.sequoiaDiscoverDrawer ? 4 : 12
        readonly property real rightPadding: controlRoot.sequoiaDiscoverDrawer ? 4 : 12
    }
''')
    patch('org/kde/desktop/ScrollView.qml', scroll_view)
    # The zero-margin applications table otherwise paints over the larger corners.
    patch('org/kde/ksysguard/page/PageContents.qml', lambda t:
          replace(replace(t,
              'anchors.margins: model.data.noMargins ? model.data.showBackground : Kirigami.Units.largeSpacing',
              'anchors.margins: model.data.showBackground ? 12 : (model.data.noMargins ? 0 : Kirigami.Units.largeSpacing)'),
              'radius: Kirigami.Units.cornerRadius', 'radius: 14'))
    from newstuff_style import build as build_newstuff
    build_newstuff(ROOT, destination, fingerprints, replace)
    from discover_islands import build as build_discover
    build_discover(ROOT, destination, fingerprints)
    from merkuro_islands import build as build_merkuro
    build_merkuro(ROOT, destination, fingerprints, replace, background)
    (destination / 'manifest.json').write_text(json.dumps({'sources': fingerprints}, indent=2) + '\n')
    return destination


def build(destination):
    destination = Path(destination)
    if destination.exists() and not (destination / 'manifest.json').exists():
        raise RuntimeError(f'Refusing to replace unmanaged directory: {destination}')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.quick-build-', dir=destination.parent) as temporary:
        prepared = _build(Path(temporary) / 'qml')
        previous = Path(temporary) / 'previous'
        if destination.exists():
            destination.rename(previous)
        try:
            prepared.rename(destination)
        except Exception:
            if previous.exists():
                previous.rename(destination)
            raise
    return destination


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / '.staging/quick-islands')
    args = parser.parse_args()
    print(build(args.output))
