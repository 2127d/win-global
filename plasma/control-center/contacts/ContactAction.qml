// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

PC.ToolButton {
    id: action
    implicitWidth: Math.max(36, implicitContentWidth + 20)
    implicitHeight: Math.max(34, implicitContentHeight + 12)
    icon.width: 18
    icon.height: 18
    icon.color: Kirigami.Theme.textColor
    padding: 8
    contentItem: RowLayout {
        spacing: 6
        Kirigami.Icon {
            source: action.icon.name
            implicitWidth: 18
            implicitHeight: 18
            isMask: true
            color: Kirigami.Theme.textColor
            Accessible.ignored: true
        }
        PC.Label {
            visible: action.display !== PC.AbstractButton.IconOnly
            text: action.text
            textFormat: Text.PlainText
            Accessible.ignored: true
        }
    }
    background: Details.DetailSurface {
        radius: 10
        emphasized: action.down
        hovered: action.hovered
        focused: action.activeFocus
    }
    PC.ToolTip { text: action.text; visible: action.hovered && action.display === PC.AbstractButton.IconOnly }
}
