// SPDX-License-Identifier: MIT
import QtQuick
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami

Item {
    id: artwork
    property alias source: cover.source
    property string fallbackIcon: "media-album-cover"
    property real cornerRadius: 20
    readonly property bool ready: cover.status === Image.Ready
    Image {
        id: cover
        objectName: "mediaArtworkImage"
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        layer.enabled: status === Image.Ready && GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: OpacityMask {
            maskSource: Item {
                width: cover.width; height: cover.height
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(cover.paintedWidth, parent.width)
                    height: Math.min(cover.paintedHeight, parent.height)
                    radius: Math.min(artwork.cornerRadius, width / 2, height / 2)
                    color: "white"
                    antialiasing: true
                }
            }
        }
    }
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height); height: width
        radius: artwork.cornerRadius
        visible: !artwork.ready
        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.1)
        Kirigami.Icon {
            anchors.centerIn: parent
            width: Math.min(64, parent.width * 0.4); height: width
            source: artwork.fallbackIcon || "media-album-cover"
            color: Kirigami.Theme.textColor
        }
    }
}
