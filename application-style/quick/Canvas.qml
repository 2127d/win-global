// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami.platform as Platform

// The application background paints the glass tint once. Nested pages must not build
// up alpha or inherit an opaque View palette across the whole application.
Rectangle {
    objectName: "sequoiaCanvas"
    Platform.Theme.colorSet: Platform.Theme.Window
    Platform.Theme.inherit: false
    property color solidColor: Platform.Theme.backgroundColor
    color: Platform.Theme.backgroundColor.a < 1 ? "transparent" : solidColor
}
