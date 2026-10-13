"""Version-checked presentation changes to Plasma's native widget explorer.

Keep KDE's catalog, installation, accessibility, keyboard and drag handlers.
Only the widget explorer floats; the activity switcher remains a sidebar.
"""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
FILES = ('contents/views/Desktop.qml', 'contents/explorer/WidgetExplorer.qml',
         'contents/explorer/AppletDelegate.qml', 'contents/explorer/PickerMetrics.qml')


def replace(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f'Plasma widget picker changed; review integration anchor: {old!r}')
    return text.replace(old, new, 1)


def desktop(text):
    text = replace(text, '    id: root\n', '    id: root\n    objectName: "sequoiaDesktopView"\n')
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
    text = replace(text, '    readonly property string pluginName:', '''    readonly property bool canAdd: !pendingUninstall && model.isSupported
    readonly property string pluginName:''')
    text = replace(text, '    Accessible.role: Accessible.Button', '''    Accessible.role: Accessible.Button
    Accessible.onPressAction: if (canAdd) widgetExplorer.addApplet(pluginName)

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
    return text


def stage(destination, system_shell):
    for name, transform in zip(FILES, (desktop, explorer, delegate)):
        (destination / name).write_text(transform((system_shell / name).read_text()))
    shutil.copy2(ROOT / 'plasma/widget-picker/PickerMetrics.qml', destination / FILES[-1])
