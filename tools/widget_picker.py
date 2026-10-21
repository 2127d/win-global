"""Version-checked grid and placement integration for Plasma's widget explorer.

Keep KDE's catalog, installation, accessibility, keyboard and drag handlers.
The widget explorer and placement dialog float; the activity switcher remains a sidebar.
Grid placement uses Plasma's native applet containers and saved geometry.
"""
from pathlib import Path
import json
import shutil

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM_FILES = ('contents/views/Desktop.qml', 'contents/explorer/WidgetExplorer.qml',
                  'contents/explorer/AppletDelegate.qml', 'contents/views/Panel.qml')
GRID_SOURCE = ROOT / 'desktop/widget-picker/grid'
GRID_FILES = tuple('contents/explorer/grid/' + p.name for p in sorted(GRID_SOURCE.iterdir()) if p.is_file())


def replace(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f'Plasma widget picker changed; review integration anchor: {old!r}')
    return text.replace(old, new, 1)


def desktop(text):
    text = replace(text, 'import "../explorer"', 'import "../explorer"\nimport "../explorer/grid"')
    text = replace(text, '    id: root\n', '    id: root\n    objectName: "sequoiaDesktopView"\n')
    text = replace(text, '    property QtObject widgetExplorer\n', '''    property QtObject widgetExplorer
    GridDesktop { containment: root.containment; view: desktop }
''')
    text = replace(text, '            if (!visible) {\n                sidePanelStack.state = "closed";',
                   '            if (!visible && GridSession.phase === "catalog") {\n                sidePanelStack.state = "closed";')
    text = replace(text, 'sidePanel.opacity = bindingWithItem(item => item.opacity, 1);',
                   'sidePanel.opacity = 1; // Content and background hide independently on Wayland.')
    text = replace(text, '                containment,\n                sidePanel,',
                   '                containment: containment || root.containment.plasmoid,\n                sidePanel,')
    text = replace(text, 'if (sidePanel.visible) {\n            if (sidePanel.sideBarOnRightEdge)',
                   'if (sidePanel.visible && !sidePanel.centeredPicker) {\n            if (sidePanel.sideBarOnRightEdge)')
    text = replace(text, '        id: sidePanel\n', '''        id: sidePanel
        objectName: "sequoiaWidgetPickerWindow"
        readonly property bool centeredPicker: sidePanelStack.state === "widgetExplorer"
        readonly property QtObject metrics: PickerMetrics {
            id: pickerMetrics
            availableWidth: sidePanel.referenceRect.width
            availableHeight: sidePanel.referenceRect.height
            frameWidth: sidePanel.margins.left + sidePanel.margins.right
            frameHeight: sidePanel.margins.top + sidePanel.margins.bottom
        }
        readonly property int pickerWidth: pickerMetrics.contentWidth
        readonly property int pickerHeight: pickerMetrics.contentHeight
''')
    text = replace(text, 'property var referenceRect: containment?.plasmoid.corona.editMode ?',
                   'property var referenceRect: (centeredPicker || containment?.plasmoid.corona.editMode) ?')
    text = replace(text, 'location: sideBarOnRightEdge ? PlasmaCore.Types.RightEdge : PlasmaCore.Types.LeftEdge',
                   'location: centeredPicker ? PlasmaCore.Types.Floating : sideBarOnRightEdge ? PlasmaCore.Types.RightEdge : PlasmaCore.Types.LeftEdge')
    text = replace(text, '            let result = desktop.x;', '''            if (centeredPicker) {
                return desktop.x + referenceRect.x + Math.floor((referenceRect.width - width) / 2);
            }
            let result = desktop.x;''')
    text = replace(text, 'y: desktop.y + (containment ? referenceRect.y : 0)',
                   'y: desktop.y + (containment ? referenceRect.y : 0) + (centeredPicker ? Math.floor((referenceRect.height - height) / 2) : 0)')
    text = replace(text, 'height: containment ? sidePanel.referenceRect.height - sidePanel.margins.top - sidePanel.margins.bottom : 1000',
                   'height: sidePanel.centeredPicker ? sidePanel.pickerHeight : containment ? sidePanel.referenceRect.height - sidePanel.margins.top - sidePanel.margins.bottom : 1000')
    text = replace(text, '                    switch (state) {', '''                    // Clear bindings left by a previous drag before switching views.
                    sidePanel.opacity = 1;
                    sidePanel.outputOnly = false;
                    switch (state) {''')
    text = replace(text, '                sidePanel.visible = true;', '''                sidePanel.visible = true;
                if (state === "widgetExplorer") {
                    sidePanel.requestActivate();
                }''')
    return text


def explorer(text):
    text = replace(text, 'import QtQuick\n', 'import QtQuick\nimport "grid"\n')
    text = replace(text, '    opacity: draggingWidget ? 0.3 : 1', '    opacity: draggingWidget || GridSession.hidden ? 0 : 1')
    text = replace(text, 'property bool preventWindowHide: draggingWidget ||',
                   'property bool preventWindowHide: draggingWidget || GridSession.phase !== "catalog" ||')
    text = replace(text, 'property bool outputOnly: draggingWidget', 'property bool outputOnly: draggingWidget || GridSession.hidden')
    text = replace(text, '    signal closed()\n', '''    signal closed()
    function beginPlacement(pluginName, name, icon) {
        GridSession.begin(pluginName, name, icon, false);
    }
    // The application Window palette carries the global theme's material alpha.
    // Kirigami's Plasma palette instead describes the common desktop theme,
    // which is shared by Sequoia's solid and transparent global themes.
    SystemPalette { id: windowPalette; colorGroup: SystemPalette.Active }
    PlacementDialog {
        id: placementDialog
        pickerWindow: main.sidePanel
        backgroundHints: windowPalette.window.a < 1 ? PlasmaCore.Dialog.StandardBackground : PlasmaCore.Dialog.SolidBackground
    }
    Connections {
        target: GridSession
        function onFinished() { main.sidePanel.requestActivate(); }
        // Unmapping a Wayland drag source cancels QDrag. Keep its transparent,
        // input-free surface mapped until Drag.onDragFinished has run.
        function onPhaseChanged() {
            main.sidePanel.visible = GridSession.phase === "catalog" || GridSession.phase === "drag";
        }
    }
    Binding {
        target: main.sidePanel
        property: "backgroundHints"
        value: GridSession.phase === "drag" ? PlasmaCore.Dialog.NoBackground
            : windowPalette.window.a < 1 ? PlasmaCore.Dialog.StandardBackground : PlasmaCore.Dialog.SolidBackground
    }
''')
    text = replace(text, '        if (!visible) {\n            KWindowSystem.showingDesktop = false',
                   '        if (!visible && GridSession.phase === "catalog") {\n            KWindowSystem.showingDesktop = false')
    text = replace(text, '    Component.onDestruction: {', '    Component.onDestruction: {\n        GridSession.cancel();')
    text = replace(text, '            widgetExplorer.addApplet(pluginName)',
                   '            beginPlacement(pluginName, list.currentItem.widgetTitle, list.currentItem.widgetIcon)')
    text = replace(text, '            if (searchInput.length > 0) {', '''            if (GridSession.phase !== "catalog") {
                GridSession.cancel();
            } else if (searchInput.length > 0) {''')
    text = text.replace('onTriggered: addCurrentApplet()', 'onTriggered: if (GridSession.phase === "catalog") addCurrentApplet()')
    text = replace(text, '    width: Math.max(heading.paintedWidth, Kirigami.Units.iconSizes.enormous * 3 + Kirigami.Units.smallSpacing * 4 + Kirigami.Units.gridUnit * 5)\n    height: 800//Screen.height', '''    objectName: "sequoiaWidgetPicker"
    width: sidePanel.pickerWidth
    height: sidePanel.pickerHeight
    background: Item {}
    readonly property bool compact: width < 620
    readonly property int previewSize: compact ? 56 : 72''')
    text = replace(text, 'readonly property int contentMargins: Kirigami.Units.largeSpacing',
                   'readonly property int contentMargins: compact ? 12 : 20')
    text = replace(text, 'var pluginName = list.currentItem ? list.currentItem.pluginName : ""',
                   'var pluginName = list.currentItem && list.currentItem.canAdd ? list.currentItem.pluginName : ""')
    text = replace(text, 'header: PlasmaExtras.PlasmoidHeading {', '''header: PlasmaExtras.PlasmoidHeading {
        background: Item {}''')
    for edge in ('top', 'left', 'right', 'bottom'):
        text = text.replace(f'main.contentMargins - main.sidePanel.margins.{edge}', 'main.contentMargins')
    text = replace(text, '            RowLayout {\n                spacing: Kirigami.Units.smallSpacing\n\n                PlasmaExtras.SearchField', '''            PC3.Label {
                Layout.fillWidth: true
                text: i18n("Click to add a widget, or drag it to your desktop or panel.")
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.WordWrap
                visible: !main.compact
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                PlasmaExtras.SearchField''')
    text = replace(text, 'text: i18ndc("plasma_shell_org.kde.plasma.desktop", "@title:group for widget grid", "Widgets")',
                   'text: i18n("Add Widgets")\n                    font.weight: Font.DemiBold')
    text = replace(text, '                    id: getWidgetsButton', '''                    id: getWidgetsButton
                    display: main.compact ? PC3.AbstractButton.IconOnly : PC3.AbstractButton.TextBesideIcon''')
    text = replace(text, '                    id: categoryButton\n', '''                    id: categoryButton
                    Layout.maximumWidth: main.width * 0.38
                    display: PC3.AbstractButton.TextBesideIcon
''')
    text = replace(text, '        id: footerContainer', '        id: footerContainer\n        background: Item {}')
    text = replace(text, '''            topMargin: -1 // account for PlasmoidHeading's pixel
            leftMargin: - main.sidePanel.margins.left
            rightMargin: - main.sidePanel.margins.right
            bottomMargin: footerContainer.visible ? 0 : - main.sidePanel.margins.bottom''', '''            topMargin: 0
            leftMargin: 0
            rightMargin: 0
            bottomMargin: 0''')
    text = replace(text, '            id: list\n', '''            id: list
            objectName: "sequoiaWidgetGrid"
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            readonly property int columns: Math.max(1, Math.floor(effectiveWidth / (main.compact ? 160 : 190)))
''')
    text = replace(text, 'cellWidth: Math.floor(effectiveWidth / 3)', 'cellWidth: Math.floor(effectiveWidth / columns)')
    text = replace(text, 'cellHeight: Kirigami.Units.iconSizes.enormous + Kirigami.Units.smallSpacing * 4 + headingFontMetrics.height * 3 * referenceHeading.lineHeight + descriptionFontMetrics.height * 2',
                   'cellHeight: main.previewSize + 40 + headingFontMetrics.height * 3 * referenceHeading.lineHeight + descriptionFontMetrics.height * 2')
    text = replace(text, '''            highlight: PlasmaExtras.Highlight {
                pressed: list.currentItem && list.currentItem.pressed
            }''', '            highlight: Item {} // Each card owns its rounded focus surface.')
    return text


def delegate(text):
    text = replace(text, 'import QtQuick\n', 'import QtQuick\nimport "grid"\n')
    text = replace(text, '    readonly property string pluginName:', '''    readonly property string widgetTitle: model.name
    readonly property string widgetIcon: model.decoration
    readonly property string pluginName:''')
    text = replace(text, '    readonly property string pluginName:', '''    readonly property bool canAdd: !pendingUninstall && model.isSupported
    readonly property string pluginName:''')
    text = replace(text, '    Accessible.role: Accessible.Button', '''    Accessible.role: Accessible.Button
    Accessible.onPressAction: if (canAdd) main.beginPlacement(pluginName, widgetTitle, widgetIcon)

    Rectangle {
        anchors.fill: parent
        anchors.margins: 5
        radius: 16
        color: delegate.pressed ? Qt.alpha(Kirigami.Theme.highlightColor, 0.22)
             : delegate.GridView.isCurrentItem ? Qt.alpha(Kirigami.Theme.highlightColor, 0.10)
             : Qt.alpha(Kirigami.Theme.textColor, 0.045)
        border.width: 1
        border.color: delegate.GridView.isCurrentItem ? Qt.alpha(Kirigami.Theme.highlightColor, 0.55)
                    : Qt.alpha(Kirigami.Theme.textColor, 0.07)
        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
    }''')
    text = replace(text, '            margins: Kirigami.Units.smallSpacing * 2',
                   '            margins: 16')
    text = replace(text, '            rightMargin: Kirigami.Units.smallSpacing * 2',
                   '            rightMargin: 16')
    text = text.replace('Kirigami.Units.iconSizes.enormous', 'main.previewSize')
    text = replace(text, 'readonly property color textColor: tapHandler.pressed ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor',
                   'readonly property color textColor: Kirigami.Theme.textColor')
    text = replace(text, 'onTapped: widgetExplorer.addApplet(delegate.pluginName)',
                   'onTapped: main.beginPlacement(delegate.pluginName, delegate.widgetTitle, delegate.widgetIcon)')
    text = replace(text, '            main.draggingWidget = true;', '''            GridSession.begin(delegate.pluginName, delegate.widgetTitle, delegate.widgetIcon, true);
            main.draggingWidget = true;''')
    text = replace(text, '            main.draggingWidget = false;', '''            main.draggingWidget = false;
            GridSession.complete();''')
    return text


def panel(text):
    text = replace(text, 'import QtQuick\n', 'import QtQuick\nimport "../explorer/grid"\n')
    return replace(text, '    property Item containment\n', '''    property Item containment
    GridTarget { containment: root.containment; view: panel }
''')


def catalog():
    widgets = {}
    for source in sorted((ROOT / 'widgets/plasmoids').iterdir()):
        metadata_path = source / 'metadata.json'
        if not metadata_path.is_file():
            continue
        metadata = json.loads(metadata_path.read_text())
        contract = metadata.get('X-Sequoia-Grid')
        if not contract:
            continue
        if contract != 'grid.json':
            raise RuntimeError(f'Unexpected grid manifest: {source}')
        definition = json.loads((source / contract).read_text())
        if metadata['KPlugin']['Id'] != source.name:
            raise RuntimeError(f'Grid widget ID must match its package directory: {source}')
        if definition.get('version') != 1 or not definition.get('presets'):
            raise RuntimeError(f'Unsupported or empty grid manifest: {source}')
        for preset in definition['presets']:
            for dimension, minimum in [('columns', 'minimumColumns'), ('rows', 'minimumRows')]:
                if type(preset.get(dimension)) is not int or not 1 <= definition[minimum] <= preset[dimension] <= 64:
                    raise RuntimeError(f'Invalid grid footprint: {source}')
        preview = definition.get('preview', '')
        if preview:
            path = (source / preview).resolve()
            if not path.is_relative_to(source.resolve()) or not path.is_file():
                raise RuntimeError(f'Invalid widget preview: {source}')
            definition['preview'] = 'previews/' + source.name + '/' + path.name
        widgets[metadata['KPlugin']['Id']] = definition
    return widgets


def preview_sources():
    result = {}
    for plugin, definition in catalog().items():
        if definition.get('preview'):
            source = ROOT / 'widgets/plasmoids' / plugin
            manifest = json.loads((source / 'grid.json').read_text())
            result['contents/explorer/grid/' + definition['preview']] = source / manifest['preview']
    return result


FILES = (*UPSTREAM_FILES, 'contents/explorer/PickerMetrics.qml', *GRID_FILES,
         'contents/explorer/grid/WidgetCatalog.js', *preview_sources())


def stage(destination, system_shell):
    for name, transform in zip(UPSTREAM_FILES, (desktop, explorer, delegate, panel)):
        (destination / name).parent.mkdir(parents=True, exist_ok=True)
        (destination / name).write_text(transform((system_shell / name).read_text()))
    shutil.copy2(ROOT / 'desktop/widget-picker/PickerMetrics.qml', destination / 'contents/explorer/PickerMetrics.qml')
    grid = destination / 'contents/explorer/grid'
    shutil.copytree(GRID_SOURCE, grid, dirs_exist_ok=True)
    (grid / 'WidgetCatalog.js').write_text('// Generated from widget grid.json manifests.\n.pragma library\nvar widgets = ' + json.dumps(catalog(), indent=2) + ';\n')
    for name, source in preview_sources().items():
        (destination / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination / name)
