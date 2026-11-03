// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailButton {
    id: button
    implicitWidth: Math.max(32, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
    topPadding: 7
    bottomPadding: 7
    leftPadding: 12
    rightPadding: 12
    font: Kirigami.Theme.defaultFont
    background: Details.DetailSurface {
        radius: Math.min(width, height) / 2
        visible: !button.flat || button.hovered || button.down || button.checked || button.visualFocus
        shadow.size: button.flat ? 0 : 5
        hovered: button.hovered && button.enabled
        pressed: button.down
        emphasized: button.checked
        focused: button.visualFocus
    }
    contentItem: RowLayout {
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
