// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

WeatherCard {
    id: root
    objectName: "weatherDetailsCard"
    required property var model
    implicitHeight: content.implicitHeight + 24
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 12; spacing: 8
        PC.Label { text: qsTr("Conditions"); font.weight: Font.DemiBold }
        Repeater {
            model: root.model
            delegate: ColumnLayout {
                id: detail
                required property var modelData
                required property int index
                objectName: "weatherDetailRow"
                Layout.fillWidth: true; spacing: 8
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Qt.alpha(Kirigami.Theme.textColor, 0.08); visible: detail.index > 0 }
                RowLayout {
                    Layout.fillWidth: true; spacing: 20
                    PC.Label {
                        Layout.fillWidth: true; Layout.preferredWidth: 1
                        text: detail.modelData.label.replace(/:$/, "")
                        color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap; textFormat: Text.PlainText
                    }
                    PC.Label {
                        Layout.fillWidth: true; Layout.preferredWidth: 1
                        text: detail.modelData.text
                        font.weight: Font.DemiBold; horizontalAlignment: Text.AlignRight
                        wrapMode: Text.Wrap; textFormat: Text.PlainText
                    }
                }
            }
        }
    }
}
