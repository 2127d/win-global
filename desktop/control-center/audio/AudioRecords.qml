// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.private.volume

Item {
    id: records
    required property var pulseModel
    required property string kind
    property var entries: []
    function refresh() {
        const next = [];
        for (let i = 0; i < objects.count; ++i) {
            const entry = objects.objectAt(i)?.snapshot;
            if (entry?.object && entry.object.name !== "auto_null") next.push(entry);
        }
        entries = next;
    }
    Instantiator {
        id: objects
        model: records.pulseModel
        delegate: QtObject {
            required property var model
            readonly property bool stream: records.kind === "sink-input" || records.kind === "source-output"
            readonly property var port: model.Ports?.[model.ActivePortIndex]
            readonly property var snapshot: ({
                id: records.kind + ":" + model.Index,
                type: records.kind,
                index: model.Index,
                deviceIndex: stream ? model.DeviceIndex : model.Index,
                object: model.PulseObject,
                name: stream ? (model.Client?.name || model.Name || qsTr("Application"))
                    : (model.Description || model.Name || qsTr("Audio device")),
                subtitle: stream ? (model.Corked ? qsTr("Inactive") : "") : (port?.description || ""),
                icon: model.IconName || (records.kind === "source" ? "audio-input-microphone-symbolic" : "audio-speakers-symbolic"),
                volume: Math.round(model.Volume / PulseAudio.NormalVolume * 100),
                muted: model.Muted,
                hasVolume: model.HasVolume !== false,
                writable: model.VolumeWritable !== false
            })
            onSnapshotChanged: Qt.callLater(records.refresh)
        }
        onObjectAdded: Qt.callLater(records.refresh)
        onObjectRemoved: Qt.callLater(records.refresh)
    }
}
