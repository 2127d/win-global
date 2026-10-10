// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Slider {
    id: slider
    implicitHeight: 26
    from: 0; to: 100; stepSize: 1
    leftPadding: 0; rightPadding: 0
    background: Rectangle {
        x: slider.leftPadding; y: (slider.height - height) / 2
        width: slider.availableWidth; height: 10; radius: 5
        color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
        Rectangle {
            x: slider.mirrored ? parent.width - width : 0
            width: slider.position * parent.width; height: parent.height; radius: 5
            color: slider.enabled ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
        }
    }
    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: (slider.height - height) / 2
        width: 18; height: 18; radius: 9
        color: Kirigami.Theme.backgroundColor
        border.width: slider.activeFocus ? 3 : 2
        border.color: slider.enabled ? Kirigami.Theme.highlightColor : Kirigami.Theme.disabledTextColor
        Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: Kirigami.Theme.highlightColor }
    }
}
