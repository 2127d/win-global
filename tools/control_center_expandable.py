# SPDX-License-Identifier: MIT
"""Retain KDE's expandable delegate with Sequoia's circular arrow feedback."""
from PySide6.QtCore import QLibraryInfo
from pathlib import Path


def build_expandable(destination):
    source = Path(QLibraryInfo.path(QLibraryInfo.QmlImportsPath)) / 'org/kde/plasma/extras/ExpandableListItem.qml'
    text = source.read_text()
    original = 'PlasmaComponents3.ToolButton {\n                    id: expandToggleButton'
    if text.count(original) != 1:
        raise RuntimeError('KDE expandable toggle changed; review its presentation integration.')
    text = text.replace(original, 'DetailToolButton {\n                    id: expandToggleButton\n                    objectName: "sequoiaExpandToggle"')
    anchor = '    property alias defaultActionButtonAction: defaultActionButton.action'
    if text.count(anchor) != 1:
        raise RuntimeError('KDE default action changed; review its presentation integration.')
    text = text.replace(anchor, anchor + '\n    readonly property alias defaultActionControl: defaultActionButton')
    text = text.replace('    readonly property alias defaultActionControl: defaultActionButton',
        '    readonly property alias defaultActionControl: defaultActionButton\n'
        '    readonly property alias expandToggleControl: expandToggleButton\n'
        '    property Component contextualActionDelegate: null\n'
        '    property real contextualActionSpacing: 0\n'
        '    property bool contextualActionsHorizontal: false\n'
        '    function closeContextualMenu() { mouseArea.contextMenu?.close(); }\n'
        '    function contextualActionAt(index) { return actionsListLoader.item?.actions.itemAt(index) ?? null; }')
    text = text.replace('sourceComponent: Item {\n                            implicitHeight: actionsListLayout.implicitHeight',
        'sourceComponent: Item {\n                            readonly property alias actions: actionRepeater\n                            implicitHeight: actionsListLayout.implicitHeight')
    text = text.replace('                                spacing: 0\n\n                                Repeater {',
        '                                spacing: listItem.contextualActionSpacing\n\n                                Repeater {')
    text = text.replace('                            ColumnLayout {\n                                id: actionsListLayout',
        '                            GridLayout {\n                                id: actionsListLayout\n'
        '                                columns: listItem.contextualActionsHorizontal ? Math.max(1, actionRepeater.count) : 1\n'
        '                                width: Math.min(implicitWidth, parent.width - 2 * Kirigami.Units.gridUnit)')
    text = text.replace('                                anchors.right: parent.right\n                                anchors.leftMargin:',
        '                                anchors.right: listItem.contextualActionsHorizontal ? undefined : parent.right\n                                anchors.leftMargin:')
    text = text.replace('                                spacing: listItem.contextualActionSpacing',
        '                                columnSpacing: listItem.contextualActionSpacing\n                                rowSpacing: listItem.contextualActionSpacing')
    start = text.index('                                    delegate: PlasmaComponents3.ToolButton {')
    end = text.index('                                        onClicked: modelData.trigger()\n                                    }', start)
    end += len('                                        onClicked: modelData.trigger()\n                                    }')
    original_delegate = text[start:end].replace('delegate: PlasmaComponents3.ToolButton {', 'PlasmaComponents3.ToolButton {', 1)
    text = text[:start] + ('                                    delegate: listItem.contextualActionDelegate || nativeContextualDelegate\n'
        '                                    property Component nativeContextualDelegate: Component {\n'
        + original_delegate + '\n                                    }') + text[end:]
    text = text.replace('PlasmaExtras.Menu {', 'DetailNativeMenu {\n                visualParent: listItem')
    destination.write_text(text)
    return source
