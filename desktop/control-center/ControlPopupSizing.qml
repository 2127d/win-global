// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts

// Intrinsic hints belong to each applet. Never read page.width/page.height:
// the tray assigns those dimensions, so using them would preserve the last size.
QtObject {
    id: sizing
    property Item page: null
    property real availableWidth: 1000
    property real availableHeight: 800
    property real headingHeight: 48
    function positive(value, fallback) {
        return Number.isFinite(value) && value > 0 ? value : fallback;
    }
    readonly property real contentWidth: positive(page?.sequoiaPreferredWidth,
        positive(page?.implicitWidth, positive(page?.Layout.preferredWidth, 418)))
    readonly property real contentHeight: positive(page?.sequoiaPreferredHeight,
        positive(page?.implicitHeight, positive(page?.Layout.preferredHeight, 380)))
    readonly property real preferredWidth: Math.max(1, Math.min(availableWidth, 680,
        Math.ceil(Math.max(320, contentWidth + 8, positive(page?.Layout.minimumWidth, 0)))))
    readonly property real preferredHeight: Math.max(1, Math.min(availableHeight, 720,
        Math.ceil(Math.max(160, contentHeight, positive(page?.Layout.minimumHeight, 0)) + headingHeight)))
}
