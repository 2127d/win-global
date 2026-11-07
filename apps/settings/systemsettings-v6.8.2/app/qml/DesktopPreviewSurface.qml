// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as K
Item {
    id: root
    required property var backdrop
    required property var desktop
    required property string material
    property real radius: 14
    property real tint: .28
    property color surfaceColor: K.Theme.backgroundColor
    ShaderEffect {
        anchors.fill: parent
        visible: root.material !== "solid" && appearancePreview.shaderPreviews
        property var source: root.backdrop
        property vector2d surfaceSize: Qt.vector2d(width, height)
        property vector2d sceneSize: Qt.vector2d(root.desktop.width, root.desktop.height)
        property vector4d sceneRect: Qt.vector4d(root.x / root.desktop.width, root.y / root.desktop.height, root.width / root.desktop.width, root.height / root.desktop.height)
        property real cornerRadius: root.radius
        property real refraction: root.material === "glass" ? 1 : 0
        fragmentShader: "qrc:/settings-hub/desktop-preview.frag.qsb"
    }
    HubPanel { anchors.fill: parent; followMaterial: false; radius: root.radius; tint: root.tint; color:Qt.alpha(root.surfaceColor,root.tint) }
}
