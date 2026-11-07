// SPDX-License-Identifier: MIT
import QtQuick

ShaderEffect {
    // Only samples wallpaper; labels, controls and applet contents are painted later.
    property var source
    property vector2d surfaceSize: Qt.vector2d(width, height)
    property real cornerRadius: Math.min(24, width / 2, height / 2)
    property real refraction: 34
    property real bevelWidth: Math.min(32, cornerRadius * 0.8)
    fragmentShader: Qt.resolvedUrl("glass-widget.frag.qsb")
}
