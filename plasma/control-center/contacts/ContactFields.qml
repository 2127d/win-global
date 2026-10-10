// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

PC.Control {
    id: root
    property string title
    property string kind
    property var fieldModel
    readonly property int count: fields.count
    signal activate(string scheme, string value)
    padding: 16
    background: Details.DetailSurface { radius: 18 }
    contentItem: ColumnLayout {
        spacing: 12
        PC.Label {
            Layout.fillWidth: true
            text: root.title
            textFormat: Text.PlainText
            font.weight: Font.DemiBold
        }
        Repeater {
            id: fields
            model: root.fieldModel
            delegate: ColumnLayout {
                id: field
                required property var model
                required property int index
                readonly property string value: root.kind === "address" ? (model.formattedAddress ?? "") : (model.display ?? "")
                Layout.fillWidth: true
                spacing: 12
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    visible: field.index > 0
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
                }
                RowLayout {
                    spacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        PC.Label {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: root.kind === "address" ? (field.model.typeLabel ?? "") : (field.model.type ?? "")
                            textFormat: Text.PlainText
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            wrapMode: Text.Wrap
                        }
                        PC.Label {
                            Layout.fillWidth: true
                            text: field.value
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                        }
                    }
                    ContactAction {
                        visible: root.kind !== "address"
                        text: root.kind === "phone" ? i18n("Call") : i18n("Send email")
                        icon.name: root.kind === "phone" ? "call-start" : "mail-message-new"
                        display: PC.AbstractButton.IconOnly
                        onClicked: root.activate(root.kind === "phone" ? "tel:" : "mailto:", field.value)
                    }
                    ContactAction {
                        visible: root.kind === "phone"
                        text: i18n("Send message")
                        icon.name: "mail-message"
                        display: PC.AbstractButton.IconOnly
                        onClicked: root.activate("sms:", field.value)
                    }
                }
            }
        }
    }
}
