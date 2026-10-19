// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

GridLayout {
    id: root
    required property var model
    columns: width >= 350 ? 2 : 1
    columnSpacing: 10
    rowSpacing: 10
    Repeater {
        model: root.model
        delegate: WeatherCard {
            id: card
            required property var modelData
            objectName: "weatherDetailCard"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            implicitHeight: content.implicitHeight + 32
            ColumnLayout {
                id: content
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8
                PC.Label {
                    Layout.fillWidth: true
                    text: card.modelData.label.replace(/:$/, "")
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                }
                PC.Label {
                    Layout.fillWidth: true
                    text: card.modelData.text
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                }
            }
        }
    }
}
