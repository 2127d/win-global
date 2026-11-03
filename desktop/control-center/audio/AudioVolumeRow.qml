// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC

RowLayout {
    id: row
    required property var record
    required property var controller
    property bool input: false
    spacing: 10
    AudioActionButton {
        objectName: "audioMute:" + row.record.id
        flat: true
        text: row.record.muted ? qsTr("Unmute %1").arg(row.record.name) : qsTr("Mute %1").arg(row.record.name)
        icon.name: row.input ? (row.record.muted ? "microphone-sensitivity-muted-symbolic" : "audio-input-microphone-symbolic")
            : (row.record.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic")
        display: PC.AbstractButton.IconOnly
        checked: !!row.record.muted
        Accessible.name: text
        onClicked: row.controller.toggleMute(row.record.id)
        PC.ToolTip { text: parent.text }
    }
    AudioLevelSlider {
        Layout.fillWidth: true
        record: row.record
        controller: row.controller
    }

    PC.Label { visible: row.record.hasVolume === false; Layout.fillWidth: true; text: qsTr("Volume controlled by app") }
}
