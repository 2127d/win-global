// SPDX-License-Identifier: MIT
import QtQuick

DisplaySlider {
    id: slider
    required property var record
    required property var controller
    objectName: "displaySlider:" + record.id
    enabled: record.maximum > 0
    to: Math.max(1,record.maximum || 0)
    value: record.brightness || 0
    stepSize: Math.max(1,Math.round(to/100))
    brightnessStep: Math.max(1,Math.round(to/100))
    Accessible.name: qsTr("Brightness for %1").arg(record.name)
    property string dragId: ""
    onMoved: controller.setBrightness(pressed ? dragId : record.id,value)
    onPressedChanged: {
        if (pressed) dragId=record.id;
        else value=Qt.binding(() => record.brightness || 0);
    }
}
