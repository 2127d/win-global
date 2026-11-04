# SPDX-License-Identifier: MIT
"""Restyle Klipper's views while retaining native history, filtering and actions."""
import shutil


def style_clipboard(destination, source, replace):
    directory = destination / 'clipboard'
    views = directory / 'views'
    for path in (source / 'clipboard').glob('*.qml'):
        shutil.copy2(path, views / path.name)

    path = views / 'ClipboardMenu.qml'
    text = path.read_text()
    text = replace(text, '    id: clipboardMenu\n', '    id: clipboardMenu\n    objectName: "clipboardHistory"\n    readonly property real naturalHeight: menuListView.count > 0 ? menuListView.contentHeight + 12 : (emptyHint.item?.implicitHeight ?? 160) + 12\n')
    text = replace(text, 'property PlasmaExtras.PlasmoidHeading header: Details.DetailHeading {', '''property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        objectName: "clipboardSearchHeader"
        background: Item {}
        leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
        leftPadding: 0
        rightPadding: 0
        topPadding: 6''')
    # Keep one horizontal rail: the representation already matches the tray heading.
    # Let the native scrollbar overlay the list rather than narrowing every card.
    text = replace(text, '    background: null\n', '    background: null\n    leftPadding: 0\n    rightPadding: 0\n')
    text = replace(text, '                id: tabBar\n', '                id: tabBar\n                spacing: 6\n                leftPadding: 0\n                rightPadding: 0\n')
    text = replace(text, 'text: i18nd("klipper", "All History")', 'leftInset: 0; rightInset: 0\n                    text: i18nd("klipper", "All History")')
    text = replace(text, 'text: i18nd("klipper", "Starred Only")', 'leftInset: 0; rightInset: 0\n                    text: i18nd("klipper", "Starred Only")')
    text = replace(text, '            spacing: 0\n', '            spacing: 8\n')
    text = replace(text, '                    id: filter\n', '''                    id: filter
                    objectName: "clipboardSearch"
                    placeholderText: qsTr("Search clipboard…")
                    implicitHeight: Math.max(42, contentHeight + 16)
                    background: Details.DetailSurface { radius: height / 2; focused: filter.activeFocus }
''')
    text = replace(text, 'enabled: menuListView.count > 0 || filter.text.length > 0',
                   'enabled: clipboardMenu.model.sourceCount > 0 || filter.text.length > 0')
    text = replace(text, 'visible: clipboardMenu.model.starredCount > 0',
                   'visible: clipboardMenu.model.sourceCount > 0 || clipboardMenu.model.starredOnly')
    text = replace(text, 'text: i18nd("klipper", "All History")', 'objectName: "clipboardHistoryTab"\n                    text: qsTr("History · %1").arg(clipboardMenu.model.sourceCount)')
    text = replace(text, 'text: i18nd("klipper", "Starred Only")', 'objectName: "clipboardStarredTab"\n                    text: qsTr("Starred · %1").arg(clipboardMenu.model.starredCount)')
    text = replace(text, '        id: metricsPlasmoidHeading\n', '        id: metricsPlasmoidHeading\n        leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0\n')
    text = replace(text, '        id: menuListView\n', '        id: menuListView\n        objectName: "clipboardList"\n        spacing: 10\n        clip: true\n')
    text = replace(text, 'leftMargin: Kirigami.Units.largeSpacing', 'leftMargin: 0')
    text = replace(text, 'rightMargin: Kirigami.Units.largeSpacing', 'rightMargin: 0')
    text = replace(text, 'header: Item { implicitHeight: Kirigami.Units.largeSpacing }', 'header: Item { implicitHeight: 6 }')
    text = replace(text, '''            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.gridUnit * 4)''', '''            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 6''')
    start = text.index('            sourceComponent: PlasmaExtras.PlaceholderMessage {', text.index('id: emptyHint'))
    end = text.index('\n            }', start) + len('\n            }')
    text = text[:start] + '''            sourceComponent: ClipboardEmptyState {
                searching: filter.text.length > 0
                starred: clipboardMenu.model.starredOnly
            }''' + text[end:]
    path.write_text(text)

    path = views / 'UrlItemDelegate.qml'
    text = replace(path.read_text(), 'previewItemDelegate.modelData.split("/")', '(previewItemDelegate.modelData ?? "").split("/")')
    text = replace(text, 'menuItem.model?.display.split(" ").length', 'menuItem.model?.display?.split(" ").length')
    path.write_text(text)

    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: main\n', '''    id: main
    Plasmoid.title: i18n("Clipboard")
    readonly property bool sequoiaFooterActions: true
''')
    text = replace(text, '    fullRepresentation: Details.DetailRepresentation {', '    fullRepresentation: PlasmaExtras.Representation {')
    text = replace(text, '        id: dialogItem\n', '''        id: dialogItem
        objectName: "sequoiaDetailPage"
        readonly property real sequoiaPreferredHeight: (header?.implicitHeight ?? 0)
            + (stack.depth === 1 ? (clipboardMenu.naturalHeight + clipboardFooter.implicitHeight + 10) : 420)
        padding: 0
        background: null
''')
    text = replace(text, 'Layout.minimumHeight: Kirigami.Units.gridUnit * 24', 'Layout.minimumHeight: 240')
    text = replace(text, 'visible: !main.isClipboardEmpty &&', 'visible: !main.expanded && !main.isClipboardEmpty &&')
    text = replace(text, 'visible: Plasmoid.containment.pluginName === "org.kde.plasma.systemtray" && page instanceof ClipboardViews.BarcodePage', 'visible: !main.expanded && Plasmoid.containment.pluginName === "org.kde.plasma.systemtray" && page instanceof ClipboardViews.BarcodePage')
    text = replace(text, '                showsClearHistoryButton: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading) && clearAction.visible', '                showsClearHistoryButton: false // The footer owns this action.')
    text = replace(text, '''            id: stack
            anchors.fill: parent''', '''            id: stack
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: clipboardFooter.visible ? clipboardFooter.top : parent.bottom
            anchors.bottomMargin: clipboardFooter.visible ? 10 : 0''')
    text = replace(text, '        QQC2.StackView {', '''        ClipboardViews.ClipboardFooter {
            id: clipboardFooter
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            clipboardMenu: stack.depth > 0 ? stack.get(0) : null
            visible: stack.depth === 1
            onConfigureRequested: klipper.configure()
        }

        Details.DetailNavigationStack {''')
    path.write_text(text)

    path = views / 'BarcodePage.qml'
    text = replace(path.read_text(), '    id: barcodeView\n', '    id: barcodeView\n    objectName: "clipboardBarcodePage"\n')
    text = replace(text, 'property PlasmaExtras.PlasmoidHeading header: Details.DetailHeading {', '''property PlasmaExtras.PlasmoidHeading header: PlasmaExtras.PlasmoidHeading {
        background: Item {}
        leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0''')
    # Keep Prison encoding, sizing, copy-image feedback and drag-and-drop intact.
    text = replace(text, 'Math.min(barcodeView.width, barcodeView.height)', 'Math.min(barcodeView.width, barcodeView.height - codeFooter.height - 12)')
    text = replace(text, '        anchors.centerIn: parent\n        width: (square', '        anchors.horizontalCenter: parent.horizontalCenter\n        anchors.top: parent.top\n        anchors.topMargin: 6\n        width: (square')
    text = replace(text, 'height: (square ? shortestSize : barcodeView.height)', 'height: (square ? shortestSize : barcodeView.height - codeFooter.height - 12)')
    at = text.rfind('\n}')
    text = text[:at] + '''
    Details.DetailSurface {
        id: codeFooter
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, codeActions.implicitWidth + 16)
        implicitHeight: codeActions.implicitHeight + 8
        radius: height / 2; elevated: true
        RowLayout {
            id: codeActions; anchors.centerIn: parent; spacing: 4
            ClipboardActionButton { objectName: "clipboardQrBack"; flat: true; text: qsTr("Back"); icon.name: "go-previous-symbolic"; onClicked: barcodeView.stack.popCurrentItem() }
            ClipboardActionButton { objectName: "clipboardQrCopy"; flat: true; action: barcodeView.copyAction }
        }
    }
''' + text[at:]
    path.write_text(text)

