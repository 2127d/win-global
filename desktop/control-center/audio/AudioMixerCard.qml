// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    id: card
    required property var record
    required property var controller
    property bool master: false
    readonly property bool input: record.type === "source-output"
    readonly property var destinations: input ? controller.inputs : controller.outputs
    readonly property var destination: destinations.find(d => d.index === record.deviceIndex) || {id:"",name:qsTr("Device unavailable")}
    objectName: master ? "audioMixerOutput" : "audioMixerApp:" + record.id
    radius: 24
    implicitHeight: body.implicitHeight + 24
    ColumnLayout {
        id: body; anchors.fill: parent; anchors.margins: 12; spacing: 8
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Kirigami.Icon { Layout.preferredWidth: 24; Layout.preferredHeight: 24; source: card.record.icon || (card.input ? "audio-input-microphone-symbolic" : "audio-speakers-symbolic") }
            Loader {
                active: card.master; visible: active; Layout.fillWidth: true
                sourceComponent: AudioDevicePicker { record: card.record; devices: card.controller.outputs; controller: card.controller }
            }
            ColumnLayout {
                visible: !card.master; Layout.fillWidth: true; spacing: 0
                PC.Label { Layout.fillWidth: true; text: card.record.name; textFormat: Text.PlainText; elide: Text.ElideRight; font.weight: Font.DemiBold }
                PC.Label { visible: card.input; text: qsTr("Microphone access"); font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9; color: Kirigami.Theme.disabledTextColor }
            }
            PC.Label { objectName: "audioPercent:" + card.record.id; text: card.record.muted ? qsTr("Muted") : qsTr("%1%").arg(Math.round(card.record.volume)); color: Kirigami.Theme.disabledTextColor }
            AudioActionButton { visible: !card.master; flat: true; text: qsTr("App audio options"); icon.name: "view-more-symbolic"; display: PC.AbstractButton.IconOnly; Accessible.name: text; onClicked: card.controller.options(card.record.id,this); PC.ToolTip { text: parent.text } }
        }
        AudioVolumeRow { Layout.fillWidth: true; record: card.record; controller: card.controller; input: card.input }
        Loader {
            Layout.fillWidth: true; active: !card.master; visible: active
            sourceComponent: AudioDevicePicker { record: card.destination; devices: card.destinations; controller: card.controller; input: card.input; streamId: card.record.id }
        }
    }
}
