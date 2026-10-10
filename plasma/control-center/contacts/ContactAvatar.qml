// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PC

Rectangle {
    id: avatar
    property string name: ""
    property url photo: ""
    readonly property color accent: Kirigami.Theme.highlightColor
    implicitWidth: 42
    implicitHeight: 42
    radius: width / 2
    color: Qt.alpha(accent, 0.12)
    PC.Label {
        anchors.centerIn: parent
        text: Components.NameUtils.initialsFromString(avatar.name)
        font.pixelSize: Math.round(avatar.width * 0.34)
        font.weight: Font.Medium
        color: Kirigami.Theme.textColor
        textFormat: Text.PlainText
        Accessible.ignored: true
    }
    Components.Avatar {
        anchors.fill: parent
        visible: avatar.photo.toString().length > 0
        name: avatar.name
        source: avatar.photo
        imageMode: Components.Avatar.AlwaysShowImage
        asynchronous: true
        Accessible.ignored: true
    }
    Accessible.ignored: true
}
