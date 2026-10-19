// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.ShadowedRectangle {
    // A Control's background can be constructed before it is parented into the
    // applet. Resolve the desktop palette directly, including on first opening.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    property bool emphasized: false
    property bool focused: false
    property bool hovered: false
    // Elevate popup panels and headers; nested controls keep their own outlines.
    property bool elevated: false
    radius: 18
    readonly property color surface: Qt.tint(Kirigami.Theme.backgroundColor,
        Kirigami.Theme.backgroundColor.r < 0.5 ? "#12ffffff" : "#70ffffff")
    color: Qt.tint(surface, Qt.alpha(Kirigami.Theme.highlightColor, emphasized ? 0.13 : hovered ? 0.06 : 0))
    border.width: focused ? 2 : elevated ? 0 : 1
    border.color: focused ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.07)
    shadow.size: elevated ? 10 : 0
    shadow.yOffset: elevated ? 2 : 0
    shadow.color: elevated ? Qt.rgba(0, 0, 0, Kirigami.Theme.backgroundColor.r < 0.5 ? 0.32 : 0.18) : "transparent"
    antialiasing: true
    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
}
