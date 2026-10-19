// SPDX-License-Identifier: MIT
import QtQuick

// Canvas clips the actual account image to a circle on GPU and software renderers.
Item {
    id: avatar
    property string name: ""
    property url source: ""
    readonly property bool hasImage: picture.ready
    Image { id: dimensions; source: avatar.source; visible: false }
    Canvas {
        id: picture
        anchors.fill: parent
        property bool ready: false
        function refresh() {
            ready = avatar.source.toString().length > 0 && isImageLoaded(avatar.source);
            if (avatar.source.toString().length > 0 && !ready) loadImage(avatar.source);
            ready = avatar.source.toString().length > 0 && isImageLoaded(avatar.source);
            requestPaint();
        }
        Component.onCompleted: refresh()
        onImageLoaded: Qt.callLater(refresh)
        onAvailableChanged: { if (available) refresh(); }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections { target: avatar; function onSourceChanged() { picture.refresh(); } }
        onPaint: {
            ready = avatar.source.toString().length > 0 && isImageLoaded(avatar.source);
            const ctx = getContext("2d");
            ctx.reset();
            ctx.beginPath(); ctx.arc(width / 2, height / 2, width / 2 - 1, 0, 2 * Math.PI); ctx.closePath();
            ctx.fillStyle = "#696775"; ctx.fill(); ctx.clip();
            if (ready) {
                const image = dimensions.sourceSize;
                const side = Math.min(image.width, image.height);
                ctx.drawImage(avatar.source, (image.width - side) / 2, (image.height - side) / 2, side, side, 0, 0, width, height);
            }
        }
    }
    Text {
        anchors.centerIn: parent
        visible: !avatar.hasImage
        text: avatar.name.trim().split(/\s+/).slice(0, 2).map(word => word.charAt(0)).join("").toUpperCase() || "?"
        color: "white"; font.pixelSize: avatar.width * 0.34; font.weight: Font.Medium
    }
}
