// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailButton {
    id: button
    implicitWidth: display === PC.AbstractButton.IconOnly ? implicitHeight : Math.max(32, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
    topPadding: 7
    bottomPadding: 7
    leftPadding: display === PC.AbstractButton.IconOnly ? 0 : 12
    rightPadding: leftPadding
    onVisualFocusChanged: if (visualFocus) Qt.callLater(revealFocus)
    function revealFocus() {
        if (!visualFocus) return;
        for (let item = parent; item; item = item.parent) {
            if (!(item instanceof Flickable)) continue;
            const pos = mapToItem(item, 0, 0);
            if (item.contentHeight > item.height) {
                const margin = Math.min(24, Math.max(0, (item.height - height) / 2));
                const dy = pos.y < margin ? pos.y - margin : Math.max(0, pos.y + height + margin - item.height);
                item.contentY = Math.max(item.originY, Math.min(item.originY + item.contentHeight - item.height, item.contentY + dy));
            }
            if (item.contentWidth > item.width) {
                const dx = pos.x < 0 ? pos.x : Math.max(0, pos.x + width - item.width);
                item.contentX = Math.max(item.originX, Math.min(item.originX + item.contentWidth - item.width, item.contentX + dx));
            }
        }
    }
    font: Kirigami.Theme.defaultFont
    background: Details.DetailSurface {
        radius: Math.min(width, height) / 2
        width: button.display === PC.AbstractButton.IconOnly ? Math.min(button.width, button.height) : button.width
        height: button.display === PC.AbstractButton.IconOnly ? width : button.height
        x: (button.width - width) / 2
        y: (button.height - height) / 2
        visible: !button.flat || button.hovered || button.down || button.checked || button.visualFocus
        shadow.size: button.flat ? 0 : 5
        hovered: button.hovered && button.enabled
        pressed: button.down
        emphasized: button.checked
        focused: button.visualFocus
    }
    contentItem: Item {
        implicitHeight: labelRow.implicitHeight
        implicitWidth: labelRow.implicitWidth
        RowLayout {
            id: labelRow
            anchors.centerIn: parent
            width: Math.min(parent.width, implicitWidth)
            spacing: 6
            Kirigami.Icon {
                visible: !!button.icon.name && button.display !== PC.AbstractButton.TextOnly
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                source: button.icon.name
                isMask: true
                color: button.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            }
            PC.Label {
                visible: button.display !== PC.AbstractButton.IconOnly
                Layout.fillWidth: true
                text: button.text
                textFormat: Text.PlainText
                font: button.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                color: button.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
            }
        }
    }
}
