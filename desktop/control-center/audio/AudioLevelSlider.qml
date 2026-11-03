// SPDX-License-Identifier: MIT
import QtQuick

AudioSlider {
        id: control
        required property var record
        required property var controller
        objectName: "audioSlider:" + control.record.id
        visible: control.record.hasVolume !== false
        enabled: control.record.writable !== false
        opacity: control.record.muted ? 0.55 : 1
        to: Math.max(control.controller.maximumVolume, control.record.volume)
        volumeStep: control.controller.volumeStep
        value: control.record.volume
        Accessible.name: qsTr("Volume for %1").arg(control.record.name)
        property bool wasPressed: false
        onMoved: {
            control.controller.setVolume(control.record.id, value);
            if (!pressed) control.controller.feedback(control.record.id);
        }
        onPressedChanged: {
            if (pressed) wasPressed = true;
            else if (wasPressed) {
                wasPressed = false;
                value = Qt.binding(() => control.record.volume);
                control.controller.feedback(control.record.id);
            }
        }
    }
