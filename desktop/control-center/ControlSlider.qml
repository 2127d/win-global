// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Slider {
    id: slider
    implicitHeight: 30
    from: 0; to: 100; stepSize: 1
    leftPadding: 0; rightPadding: 0
    background: Rectangle {
        x: slider.leftPadding; y: (slider.height - height) / 2
        width: slider.availableWidth; height: 24; radius: height / 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
        Rectangle {
            x: slider.mirrored ? parent.width - width : 0
            width: slider.handle.width / 2 + slider.position * (parent.width - slider.handle.width); height: parent.height; radius: height / 2
            color: slider.enabled ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
        }
    }
    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: (slider.height - height) / 2
        width: 24; height: 24; radius: width / 2
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
        border.width: slider.activeFocus ? 2 : 1
        border.color: slider.activeFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.16)
    }
}
