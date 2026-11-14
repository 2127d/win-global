"""Version-checked presentation of KDE's shared Get New browser.

Keep the native Engine, actions, entry delegates and safety/install questions.
Only presentation is patched; QML and QWidget DownloadDialogs share this module.
"""
import shutil
import subprocess


def build(root, destination, fingerprints, replace):
    module = destination / 'org/kde/newstuff'
    library = (root / 'desktop/components/library').resolve()
    build_dir = library / '.build'
    subprocess.run(['cmake', '-S', str(library), '-B', str(build_dir), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=Release'], check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['cmake', '--build', str(build_dir), '--parallel', '4'], check=True,
                   stdout=subprocess.DEVNULL)
    shared = build_dir / 'bin/qml/org/sequoia/components'
    if not (shared / 'libsequoiacomponentsplugin.so').is_file():
        raise RuntimeError('Build the shared component library before the Quick overlay.')
    shutil.copytree(shared, destination / 'org/sequoia/components', dirs_exist_ok=True)
    shutil.copy2(root / 'themes/application-style/newstuff/SequoiaHeader.qml', module / 'private/SequoiaHeader.qml')

    def patch(name, transform):
        path = module / name
        path.write_text(transform(path.read_text()))

    def imports(t):
        return replace(t, 'import QtQuick\n', 'import QtQuick\nimport org.sequoia.components as S\n')

    patch('Dialog.qml', lambda t: replace(t, '    color: Kirigami.Theme.backgroundColor', '''    // Paint the tint once, as a scene item, matching our application windows.
    color: "transparent"
    Rectangle { anchors.fill: parent; color: Kirigami.Theme.backgroundColor }
    objectName: "sequoiaNewStuffDialog"'''))
    patch('DialogContent.qml', lambda t: replace(t,
          'pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.Auto',
          'pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None'))

    def page(t):
        t = imports(t)
        t = replace(t, '    id: root', '''    id: root
    objectName: "sequoiaNewStuffPage"
    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None
    background: Rectangle { color: "transparent" }
    leftPadding: 16
    rightPadding: 16
    topPadding: 12
    bottomPadding: 12
    view.cellWidth: Math.min(view.width, view.implicitCellWidth + 16)''')
        t = replace(t, '    header: Kirigami.InlineMessage {', '''    header: ColumnLayout {
        spacing: 12
        Private.SequoiaHeader {
            page: root
            stack: pageStack
            Layout.fillWidth: true
            Layout.margins: 16
        }
        Kirigami.InlineMessage {
        background: S.Panel { radius: 16 }
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.bottomMargin: 12''')
        t = replace(t, '\n    NewStuff.Engine {', '\n    }\n\n    NewStuff.Engine {')
        t = replace(t, 'displayComponent: Kirigami.SearchField {', 'displayComponent: S.SearchField {')
        t = replace(t, '                focusSequence: "Ctrl+F"', '''                implicitWidth: 240
                Shortcut { sequence: "Ctrl+F"; onActivated: searchField.forceActiveFocus() }''')
        t = replace(t, '        QQC2.ComboBox {', '''        S.ThemeSelector {
            backdropItem: root
            Accessible.name: i18nd("knewstuff6", "Category:")
            Layout.maximumWidth: 360''')
        t = replace(t, '        QQC2.Button {', '        S.ActionPill {')
        return t
    patch('Page.qml', page)
    def details(t):
        t = imports(t)
        t = replace(t, '    id: component', '''    id: component
    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None
    background: Rectangle { color: "transparent" }
    header: Private.SequoiaHeader {
        page: component
        stack: pageStack
        leftPadding: 0
    }'''.replace('        leftPadding: 0\n',''))
        return t.replace('QQC2.Button {', 'S.ActionPill {')
    patch('EntryDetails.qml', details)
    # Native question/error lifecycle stays intact; use the library's surface.
    for name in ('private/ErrorDisplayer.qml', 'DownloadItemsSheet.qml'):
        def dialog(t):
            t = imports(t)
            t = replace(t, '    id: component', "    id: component\n    background: S.Panel { radius: 24 }")
            return t.replace('QQC2.ToolButton {', 'S.ActionPill {')
        patch(name, dialog)

    def card(t):
        t = imports(t)
        t = replace(t, '    Kirigami.ShadowedRectangle {', '    S.Panel {')
        t = replace(t, '\n        anchors.centerIn: parent\n', '\n        x: 0; y: 8\n')
        t = replace(t, '\n        radius: Kirigami.Units.smallSpacing\n', '\n        radius: 20\n')
        t = replace(t, '''        shadow.xOffset: 0
        shadow.yOffset: 2
        shadow.size: 10
        shadow.color: Qt.rgba(0, 0, 0, 0.3)''', '')
        # Panel supplies the theme-aware surface and native rim.
        start = t.index('        color: {')
        end = t.index('\n        Rectangle {', start)
        t = t[:start] + '''        emphasized: delegate.hovered || delegate.visualFocus
''' + t[end:]
        t = replace(t, 'radius: Kirigami.Units.smallSpacing/2', 'radius: 12')
        t = replace(t, '            color: Kirigami.Theme.backgroundColor', '            color: "transparent"')
        t = replace(t, '            radius: Kirigami.Units.smallSpacing', '            radius: 12')
        return t.replace('delegate: QQC2.Button {', 'delegate: S.ActionPill {')
    patch('private/GridTileDelegate.qml', card)
    # Preserve the native preview/image loader and crop mode; just round its mask.
    def preview(t):
        t = replace(t, 'import QtQuick\n', 'import QtQuick\nimport QtQuick.Effects\n')
        t = replace(t, '                    visible: bigPreview.status === Image.Ready',
                    '                    radius: 12\n                    color: Kirigami.Theme.backgroundColor\n                    visible: bigPreview.status === Image.Ready')
        t = replace(t, 'source: thumbnailAvailable ? model.previews[0] : ""',
                    'source: model.previews.length > 0 ? model.previews[0] : (model.previewsSmall.length > 0 ? model.previewsSmall[0] : "")')
        return replace(t, '                Image {\n                    id: bigPreview', '''                Rectangle {
                    id: bigPreviewMask
                    width: bigPreview.width; height: bigPreview.height
                    radius: 12; color: "white"; layer.enabled: true; visible: false
                }
                Image {
                    id: bigPreview
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        maskEnabled: true
                        autoPaddingEnabled: false
                        maskSource: bigPreviewMask
                    }''')
    patch('private/entrygriddelegates/BigPreviewDelegate.qml', preview)
