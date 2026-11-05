// SPDX-License-Identifier: MIT
import QtQuick
import QtQml.Models
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

DetailInsetMenu {
    id: menu
    required property var nativeActions
    objectName: "sequoiaHeaderActionMenu"
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent

    implicitHeight: {
        let rowsHeight = Math.max(0, count - 1) * contentItem.spacing;
        for (let i = 0; i < count; ++i)
            rowsHeight += itemAt(i)?.implicitHeight ?? 0;
        return rowsHeight + topPadding + bottomPadding;
    }

    function openFor(button) {
        popup(button, button.mirrored ? 0 : button.width - width, button.height + 4);
    }

    background: DetailSurface {
        radius: 24
        elevated: true
        color: stateColor
    }

    Instantiator {
        model: menu.nativeActions
        delegate: T.MenuItem {
            id: row
            required property var modelData
            readonly property bool separator: modelData?.isSeparator ?? false
            text: modelData?.text ?? ""
            Kirigami.MnemonicData.label: text
            Accessible.name: Kirigami.MnemonicData.plainTextLabel
            enabled: !separator && (modelData?.enabled ?? false)
            checkable: modelData?.checkable ?? false
            checked: modelData?.checked ?? false
            hoverEnabled: enabled
            leftPadding: 12
            rightPadding: 12
            topPadding: separator ? 4 : 7
            bottomPadding: topPadding
            implicitWidth: separator ? 0 : metrics.advanceWidth + 50
            implicitHeight: separator ? 9 : Math.max(36, label.implicitHeight + topPadding + bottomPadding)
            indicator: null
            arrow: null
            TextMetrics { id: metrics; font: label.font; text: label.text }
            contentItem: Item {
                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                LayoutMirroring.childrenInherit: true
                Rectangle {
                    visible: row.separator
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 1
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
                }
                Kirigami.Icon {
                    id: symbol
                    visible: !row.separator
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18; height: 18
                    source: row.checked ? "checkmark-symbolic" : row.modelData?.icon?.name ?? ""
                    isMask: true
                    color: label.color
                }
                PC.Label {
                    id: label
                    visible: !row.separator
                    anchors.left: symbol.right
                    anchors.leftMargin: 8
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.Kirigami.MnemonicData.plainTextLabel
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: row.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                }
            }
            background: Rectangle {
                radius: height / 2
                visible: row.enabled && (row.highlighted || row.hovered || row.down || row.visualFocus || row.checked)
                color: Qt.alpha(Kirigami.Theme.highlightColor, row.down ? 0.25 : row.hovered || row.highlighted ? 0.20 : 0.12)
                border.color: Kirigami.Theme.highlightColor
                border.width: row.visualFocus ? 1 : 0
            }
            PC.ToolTip { text: label.text; visible: row.hovered && label.truncated; delay: Kirigami.Units.toolTipDelay }
            onTriggered: modelData.trigger()
        }
        onObjectAdded: (index, row) => menu.insertItem(index, row)
        onObjectRemoved: (index, row) => menu.removeItem(row)
    }
}
