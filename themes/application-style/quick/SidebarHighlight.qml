// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami.platform as Platform
import org.kde.kirigami as Kirigami

Kirigami.ShadowedRectangle {
    id: pill
    objectName: "sequoiaSidebarHighlight"
    required property T.ItemDelegate control
    readonly property bool selected: control.checked || control.highlighted
    readonly property bool dark: Platform.Theme.textColor.hslLightness > 0.5
    readonly property int duration: Platform.Units.longDuration > 0 ? 160 : 0
    radius: height / 2
    color: selected ? Qt.alpha(Platform.Theme.highlightColor, dark ? 0.22 : 0.15)
        : Qt.alpha(Platform.Theme.textColor, control.down ? 0.10 : control.hovered ? 0.055 : 0)
    border.width: control.visualFocus ? 2 : 1
    border.color: Qt.alpha(Platform.Theme.highlightColor, control.visualFocus ? 0.8 : selected ? 0.25 : 0)
    shadow.size: 6
    shadow.xOffset: 0
    shadow.yOffset: 2
    shadow.color: Qt.rgba(0, 0, 0, selected ? (dark ? 0.24 : 0.11) : 0)
    Behavior on color { ColorAnimation { duration: pill.duration; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: pill.duration; easing.type: Easing.OutCubic } }
}
