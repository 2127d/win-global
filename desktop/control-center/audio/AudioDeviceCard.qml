// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    required property var record
    required property var devices
    required property var controller
    property bool input: false
    implicitHeight: body.implicitHeight + 28
    radius: 24
    objectName: input ? "audioInputCard" : "audioOutputCard"
    ColumnLayout {
        id: body
        anchors.fill: parent; anchors.margins: 14
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            visible: !card.input
            Rectangle {
                visible: !card.input
                implicitWidth: badge.implicitWidth + 18; implicitHeight: badge.implicitHeight + 8
                radius: height / 2; color: Qt.alpha(Kirigami.Theme.highlightColor,0.16)
                PC.Label { id: badge; anchors.centerIn: parent; text: qsTr("Sound output"); font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9 }
            }
            PC.Label { visible: card.input; text: qsTr("Microphone"); font.weight: Font.DemiBold }
            Item { Layout.fillWidth: true }
            PC.Label {
                objectName: card.input ? "" : "audioPercent:" + card.record.id
                text: card.record.muted ? qsTr("Muted") : qsTr("%1%").arg(Math.round(card.record.volume))
                color: Kirigami.Theme.disabledTextColor
            }
            AudioActionButton {
                objectName: card.input ? "" : "audioOptions:" + card.record.id
                flat: true; icon.name: "view-more-symbolic"; display: PC.AbstractButton.IconOnly
                text: qsTr("Device options"); Accessible.name: text
                onClicked: card.controller.options(card.record.id,this)
                PC.ToolTip { text: parent.text }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Rectangle {
                Layout.preferredWidth: card.input ? 36 : 58; Layout.preferredHeight: width
                radius: card.input ? width / 2 : 20
                color: Qt.alpha(Kirigami.Theme.highlightColor,0.18)
                Kirigami.Icon {
                    anchors.centerIn: parent; width: card.input ? 19 : 30; height: width
                    source: card.input ? "audio-input-microphone-symbolic" : (card.record.icon || "audio-speakers-symbolic")
                    isMask: true; color: Kirigami.Theme.textColor
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 2
                PC.Label { visible: !card.input || card.devices.length < 2; Layout.fillWidth: true; text: card.record.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight; font.weight: Font.DemiBold; font.pointSize: Kirigami.Theme.defaultFont.pointSize * (card.input ? 1 : 1.3) }
                Loader {
                    Layout.fillWidth: true; active: card.input && card.devices.length > 1; visible: active
                    sourceComponent: AudioDevicePicker { record: card.record; devices: card.devices; controller: card.controller; input: true }
                }
                PC.Label { Layout.fillWidth: true; visible: card.input || !!card.record.subtitle; text: card.input ? qsTr("Microphone") : (card.record.subtitle || ""); textFormat: Text.PlainText; elide: Text.ElideRight; color: Kirigami.Theme.disabledTextColor; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9 }
            }
            PC.Label {
                visible: card.input
                objectName: card.input ? "audioPercent:" + card.record.id : ""
                text: card.record.muted ? qsTr("Muted") : qsTr("%1%").arg(Math.round(card.record.volume))
                color: Kirigami.Theme.disabledTextColor
            }
            AudioActionButton {
                objectName: card.input ? "audioOptions:" + card.record.id : ""
                visible: card.input
                flat: true; icon.name: "view-more-symbolic"; display: PC.AbstractButton.IconOnly
                text: qsTr("Microphone options"); Accessible.name: text
                onClicked: card.controller.options(card.record.id,this)
                PC.ToolTip { text: parent.text }
            }
        }
        AudioVolumeRow { Layout.fillWidth: true; record: card.record; controller: card.controller; input: card.input }
        Loader {
            Layout.fillWidth: true; active: !card.input; visible: active
            sourceComponent: AudioDevicePicker { record: card.record; devices: card.devices; controller: card.controller }
        }
    }
}
