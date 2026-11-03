// SPDX-License-Identifier: MIT
import QtQuick

Item {
    id: records
    required property var displayModel
    property var entries: []
    property var numbering: ({})
    property int nextNumber: 1
    function refresh() {
        const next = [];
        for (let i=0; i<objects.count; ++i) {
            const record = objects.objectAt(i)?.snapshot;
            if (!record) continue;
            if (numbering[record.id] === undefined) numbering[record.id] = nextNumber++;
            next.push(Object.assign({},record,{number:numbering[record.id]}));
        }
        entries = next.sort((a,b) => a.number-b.number);
    }
    Instantiator {
        id: objects
        model: records.displayModel
        delegate: QtObject {
            required property string displayName
            required property string label
            required property int brightness
            required property int maxBrightness
            readonly property var snapshot: ({
                id: displayName + "|" + label,
                backendId: displayName,
                name: label || qsTr("Display"),
                brightness: brightness,
                maximum: maxBrightness
            })
            onSnapshotChanged: Qt.callLater(records.refresh)
        }
        onObjectAdded: Qt.callLater(records.refresh)
        onObjectRemoved: Qt.callLater(records.refresh)
    }
}
