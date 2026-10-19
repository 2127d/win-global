// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

// Paint only the indicator: native Switch/SwitchDelegate retain input, drag,
// accessibility, focus, checked state, and right-to-left handling.
Item {
    id: indicator
    property T.AbstractButton control
    property alias handle: thumb
    // MenuItem is an AbstractButton without Switch's position properties.
    readonly property real visualPosition: typeof control.visualPosition === "number"
        ? control.visualPosition : (control.checked !== control.mirrored ? 1 : 0)
    implicitHeight: Math.round(Kirigami.Units.gridUnit * 1.2)
    implicitWidth: Math.round(implicitHeight * 1.75)
    readonly property real inset: Math.max(2, height / 12)
    // Keep the actual accent for disabled switches; opacity supplies their
    // disabled treatment instead of KDE's background-colored selection role.
    SystemPalette { id: activePalette; colorGroup: SystemPalette.Active }
    readonly property color accent: control.enabled ? control.Kirigami.Theme.highlightColor : activePalette.highlight
    readonly property color surface: control.enabled ? control.Kirigami.Theme.backgroundColor : activePalette.window
    readonly property color foreground: control.enabled ? control.Kirigami.Theme.textColor : activePalette.windowText
    readonly property color offColor: Qt.tint(Qt.rgba(surface.r, surface.g, surface.b, 1), Qt.alpha(foreground, 0.19))
    opacity: control.enabled ? 1 : 0.45
    layer.enabled: opacity < 1

    Rectangle {
        objectName: "sequoiaSwitchFocus"
        anchors.fill: parent
        anchors.margins: -3
        radius: height / 2
        color: "transparent"
        border.width: 2
        border.color: indicator.accent
        visible: indicator.control.visualFocus
    }
    Rectangle {
        id: track
        objectName: "sequoiaSwitchTrack"
        anchors.fill: parent
        radius: height / 2
        color: indicator.control.checked ? indicator.accent : indicator.offColor
        Behavior on color {
            enabled: Kirigami.Units.shortDuration > 0
            ColorAnimation { duration: Kirigami.Units.shortDuration }
        }
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: indicator.control.down ? "#14000000" : "transparent"
        }
    }
    Rectangle {
        x: thumb.x
        y: thumb.y + 1
        width: thumb.width; height: thumb.height
        radius: height / 2
        color: "#30000000"
    }
    Rectangle {
        id: thumb
        objectName: "sequoiaSwitchThumb"
        width: parent.height - 2 * indicator.inset
        height: width
        radius: height / 2
        y: indicator.inset
        x: indicator.inset + indicator.visualPosition * (parent.width - width - 2 * indicator.inset)
        color: "white"
        Behavior on x {
            enabled: !indicator.control.down && Kirigami.Units.shortDuration > 0
            NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutCubic }
        }
    }
}
