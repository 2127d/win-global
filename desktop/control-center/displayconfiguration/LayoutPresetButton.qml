// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

PC.Button {
    id: button
    required property var preset
    text: preset.label || ""
    Accessible.name: text
    implicitHeight: Math.max(66, body.implicitHeight + 24)
    leftPadding: 12; rightPadding: 12; topPadding: 12; bottomPadding: 12
    background: Details.DetailSurface {
        radius: 16
        emphasized: button.down
        hovered: button.hovered && button.enabled
        focused: button.visualFocus
        opacity: button.enabled ? 1 : 0.55
    }
    contentItem: RowLayout {
        id: body
        spacing: 10
        opacity: button.enabled ? 1 : 0.5
        Kirigami.Icon {
            Layout.preferredWidth: 32; Layout.preferredHeight: 32
            source: button.preset.iconName || "video-display-symbolic"
        }
        PC.Label {
            Layout.fillWidth: true
            text: button.text
            textFormat: Text.PlainText; wrapMode: Text.Wrap
            font.weight: Font.Medium
        }
    }
    PC.ToolTip { text: button.text }
}
