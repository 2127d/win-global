// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

// Content material only. PlasmaCore.Dialog retains ownership of the window's
// blur, shadow, input region and opening/closing lifecycle.
Kirigami.ShadowedRectangle {
    id: surface
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    property bool panel: false
    property bool elevated: false
    property bool glass: false
    property bool emphasized: false
    property bool focused: false
    property bool hovered: false
    property bool pressed: false
    property int motionDuration: Kirigami.Units.shortDuration > 1 ? Kirigami.Units.shortDuration : 0
    readonly property bool dark: Kirigami.Theme.backgroundColor.hslLightness < 0.5
    // Interior islands stay opaque even when the application palette has alpha.
    readonly property color base: Qt.alpha(Kirigami.Theme.backgroundColor, 1)
    readonly property color material: Qt.tint(base, dark ? "#12ffffff" : "#70ffffff")
    readonly property color stateColor: Qt.tint(material, Qt.alpha(Kirigami.Theme.highlightColor,
        emphasized ? Tokens.selectedOpacity : pressed ? Tokens.pressedOpacity : hovered ? Tokens.hoverOpacity : 0))
    radius: panel ? Tokens.panelRadius : Tokens.cardRadius
    color: Qt.alpha(stateColor, glass ? Tokens.glassOpacity : 1)
    border.width: focused ? Tokens.focusWidth : elevated ? 0 : 1
    border.color: focused ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, Tokens.rimOpacity)
    shadow.size: elevated ? Tokens.shadowSize : 0
    shadow.yOffset: elevated ? Tokens.shadowOffset : 0
    shadow.color: elevated ? Qt.rgba(0, 0, 0, dark ? 0.32 : 0.18) : "transparent"
    antialiasing: true
    Behavior on color { ColorAnimation { duration: surface.motionDuration } }
}
