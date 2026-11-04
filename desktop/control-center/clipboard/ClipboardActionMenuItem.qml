// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../../details" as Details

T.MenuItem {
    id: item
    property bool section: false
    implicitWidth: textSize.width + 64
    implicitHeight: Math.max(section ? 28 : 36, label.implicitHeight + (section ? 8 : 14))
    leftPadding: 12; rightPadding: 12; topPadding: 7; bottomPadding: 7
    font: Kirigami.Theme.defaultFont
    hoverEnabled: !section
    enabled: !section
    indicator: null
    arrow: null
    TextMetrics { id: textSize; font: item.font; text: item.text }
    contentItem: RowLayout {
        spacing: 8
        Kirigami.Icon {
            visible: !item.section
            Layout.preferredWidth: 18; Layout.preferredHeight: 18
            source: item.checked ? "checkmark-symbolic" : item.icon.name
            isMask: true
            color: item.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
        }
        PC.Label {
            id: label
            Layout.fillWidth: true
            text: item.text; textFormat: Text.PlainText
            font: item.font
            elide: Text.ElideRight
            color: item.section || !item.enabled ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
        }
        Kirigami.Icon {
            visible: !!item.subMenu
            Layout.preferredWidth: 16; Layout.preferredHeight: 16
            source: item.mirrored ? "go-previous-symbolic" : "go-next-symbolic"
            isMask: true; color: Kirigami.Theme.textColor
        }
    }
    background: Details.DetailSurface {
        objectName: "clipboardMenuHighlight"
        radius: height / 2
        shadow.size: 0
        visible: !item.section && (item.checked || item.highlighted || item.hovered || item.down || item.visualFocus)
        color: Qt.alpha(Kirigami.Theme.highlightColor, item.down ? 0.25 : item.highlighted || item.hovered ? 0.20 : 0.12)
        focused: item.visualFocus
    }
    PC.ToolTip { text: item.text; visible: item.hovered && label.truncated; delay: Kirigami.Units.toolTipDelay }
}
