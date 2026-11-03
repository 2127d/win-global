// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    id: card
    required property var record
    required property var devices
    required property var controller
    property bool input: false
    objectName: input ? "audioInputChannel" : "audioOutputChannel"
    radius: 24
    implicitHeight: body.implicitHeight + 24
    ColumnLayout {
        id: body
        anchors.fill: parent; anchors.margins: 12
        spacing: 8
        RowLayout {
            Layout.alignment: Qt.AlignHCenter; spacing: 6
            Kirigami.Icon { Layout.preferredWidth: 20; Layout.preferredHeight: 20; source: card.input ? "audio-input-microphone-symbolic" : (card.record.icon || "audio-speakers-symbolic"); isMask: true; color: Kirigami.Theme.textColor }
            PC.Label { text: card.input ? qsTr("Input") : qsTr("Output"); font.weight: Font.DemiBold }
        }
        PC.Label { Layout.alignment: Qt.AlignHCenter; objectName: "audioPercent:" + card.record.id; text: card.record.muted ? qsTr("Muted") : qsTr("%1%").arg(Math.round(card.record.volume)); color: Kirigami.Theme.disabledTextColor }
        AudioLevelSlider {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 128; Layout.preferredWidth: 54
            orientation: Qt.Vertical
            record: card.record; controller: card.controller
        }
        AudioDevicePicker { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitWidth: 1; record: card.record; devices: card.devices; controller: card.controller; input: card.input }
        AudioActionButton {
            objectName: "audioMute:" + card.record.id
            Layout.alignment: Qt.AlignHCenter; flat: true
            text: card.record.muted ? qsTr("Unmute") : qsTr("Mute")
            checked: !!card.record.muted
            icon.name: card.input ? (card.record.muted ? "microphone-sensitivity-muted-symbolic" : "audio-input-microphone-symbolic") : (card.record.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic")
            Accessible.name: text + " " + card.record.name
            onClicked: card.controller.toggleMute(card.record.id)
        }
    }
}
