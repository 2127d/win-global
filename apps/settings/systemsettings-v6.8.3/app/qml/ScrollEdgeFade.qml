// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Effects

// Fade the viewport's alpha, preserving the actual material behind it and all
// native wheel, touch and keyboard interaction on the original Flickable.
Item {
    id: fade
    required property Item viewport
    parent: viewport ? viewport.parent : null
    property real fadeSize: 24
    property bool shaderEnabled: appearancePreview.shaderPreviews
    readonly property real offsetX: viewport ? viewport.contentX - (viewport.originX ?? 0) : 0
    readonly property real offsetY: viewport ? viewport.contentY - (viewport.originY ?? 0) : 0
    readonly property bool leftHidden: viewport && offsetX > .5
    readonly property bool rightHidden: viewport && viewport.contentWidth - viewport.width - offsetX > .5
    readonly property bool topHidden: viewport && offsetY > .5
    readonly property bool bottomHidden: viewport && viewport.contentHeight - viewport.height - offsetY > .5
    readonly property bool fading: shaderEnabled && (leftHidden || rightHidden || topHidden || bottomHidden)
    onLeftHiddenChanged: mask.requestPaint()
    onRightHiddenChanged: mask.requestPaint()
    onTopHiddenChanged: mask.requestPaint()
    onBottomHiddenChanged: mask.requestPaint()
    onFadeSizeChanged: mask.requestPaint()

    Canvas {
        id: mask
        width: fade.viewport ? fade.viewport.width : 0
        height: fade.viewport ? fade.viewport.height : 0
        visible: false
        layer.enabled: fade.fading
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            if (width <= 0 || height <= 0) return;
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.globalCompositeOperation = "source-over";
            ctx.fillStyle = "white";
            ctx.fillRect(0, 0, width, height);
            ctx.globalCompositeOperation = "destination-in";
            const vertical = ctx.createLinearGradient(0, 0, 0, height);
            const fy = Math.min(.5, fade.fadeSize / height);
            vertical.addColorStop(0, fade.topHidden ? "transparent" : "white");
            vertical.addColorStop(fy, "white");
            vertical.addColorStop(1 - fy, "white");
            vertical.addColorStop(1, fade.bottomHidden ? "transparent" : "white");
            ctx.fillStyle = vertical;
            ctx.fillRect(0, 0, width, height);
            const horizontal = ctx.createLinearGradient(0, 0, width, 0);
            const fx = Math.min(.5, fade.fadeSize / width);
            horizontal.addColorStop(0, fade.leftHidden ? "transparent" : "white");
            horizontal.addColorStop(fx, "white");
            horizontal.addColorStop(1 - fx, "white");
            horizontal.addColorStop(1, fade.rightHidden ? "transparent" : "white");
            ctx.fillStyle = horizontal;
            ctx.fillRect(0, 0, width, height);
        }
    }
    property Component fadeEffect: Component {
        MultiEffect {
            maskEnabled: true
            maskSource: mask
            // Preserve the gradual alpha ramp instead of thresholding it.
            maskThresholdMin: .5
            maskSpreadAtMin: 1
            maskThresholdMax: 1
            maskSpreadAtMax: 0
            autoPaddingEnabled: false
        }
    }
    Binding { target: fade.viewport; property: "clip"; value: true }
    Binding { target: fade.viewport ? fade.viewport.layer : null; property: "enabled"; value: fade.fading }
    Binding { target: fade.viewport ? fade.viewport.layer : null; property: "effect"; value: fade.fadeEffect }
}
