// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import plasma.applet.org.kde.plasma.weather as Weather

Column {
    id: root
    property var model
    spacing: 12
    Repeater {
        model: root.model
        delegate: WeatherCard {
            id: card
            required property var model
            objectName: "weatherNoticeCard"
            width: root.width
            implicitHeight: content.implicitHeight + 32
            readonly property color accent: model.priority === Weather.Warnings.High ? Kirigami.Theme.negativeTextColor
                : model.priority === Weather.Warnings.Medium ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.highlightColor
            ColumnLayout {
                id: content
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle {
                        Layout.preferredWidth: 32; Layout.preferredHeight: 32
                        radius: 10
                        color: Qt.alpha(card.accent, 0.12)
                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: 20; height: 20
                            source: card.model.priority >= Weather.Warnings.Medium ? "data-warning-symbolic" : "data-information-symbolic"
                            isMask: true
                            color: card.accent
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        PC.Label {
                            text: card.model.priority === Weather.Warnings.High ? qsTr("Weather warning")
                                : card.model.priority === Weather.Warnings.Medium ? qsTr("Weather advisory") : qsTr("Weather notice")
                            font.weight: Font.DemiBold
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                        PC.Label {
                            text: card.model.timestamp || ""
                            visible: text.length > 0
                            color: Kirigami.Theme.disabledTextColor
                            font: Kirigami.Theme.smallFont
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                        }
                    }
                }
                Kirigami.SelectableLabel {
                    Layout.fillWidth: true
                    text: card.model.description || ""
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
                PC.Button {
                    text: qsTr("More information")
                    icon.name: "window-new-symbolic"
                    visible: !!card.model.info
                    onClicked: Qt.openUrlExternally(card.model.info)
                }
            }
        }
    }
}
