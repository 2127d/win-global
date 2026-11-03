// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Slider {
    id: slider
    property int brightnessStep: 5
    implicitHeight: horizontal ? 32 : 128
    implicitWidth: horizontal ? 180 : 54
    from: 0; to: 100; stepSize: 1
    leftPadding: 0; rightPadding: 0
    wheelEnabled: false
    WheelHandler {
        acceptedButtons: Qt.NoButton
        property int remainder: 0
        onWheel: event => {
            remainder += (event.angleDelta.y || -event.angleDelta.x) * (event.inverted ? -1 : 1);
            const steps = Math.trunc(remainder / 120);
            if (steps) {
                remainder -= steps * 120;
                slider.value = Math.max(slider.from, Math.min(slider.to, slider.value + steps * slider.brightnessStep));
                slider.moved();
            }
        }
    }
    background: Rectangle {
        x: slider.horizontal ? slider.leftPadding : (slider.width - width) / 2
        y: slider.horizontal ? (slider.height - height) / 2 : slider.topPadding
        width: slider.horizontal ? slider.availableWidth : 48
        height: slider.horizontal ? 20 : slider.availableHeight
        radius: Math.min(width,height) / 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.11)
        Rectangle {
            x: slider.horizontal && slider.mirrored ? parent.width - width : 0
            y: slider.horizontal ? 0 : parent.height - height
            width: slider.horizontal ? slider.handle.width / 2 + slider.position * (parent.width - slider.handle.width) : parent.width
            height: slider.horizontal ? parent.height : slider.handle.height / 2 + slider.position * (parent.height - slider.handle.height)
            radius: Math.min(parent.width,parent.height) / 2
            color: slider.enabled ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
        }
    }
    handle: Rectangle {
        x: slider.horizontal ? slider.leftPadding + slider.visualPosition * (slider.availableWidth - width) : (slider.width - width) / 2
        y: slider.horizontal ? (slider.height - height) / 2 : slider.topPadding + (1 - slider.position) * (slider.availableHeight - height)
        width: slider.horizontal ? 24 : 42; height: slider.horizontal ? 24 : 14; radius: height / 2
        color: Kirigami.Theme.backgroundColor
        border.width: slider.activeFocus ? 2 : 1
        border.color: slider.activeFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor,0.18)
    }
}
