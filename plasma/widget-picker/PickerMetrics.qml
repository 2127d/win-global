// SPDX-License-Identifier: MIT
import QtQml

QtObject {
    // Plasma supplies logical pixels, so fractional/HiDPI scaling is already
    // accounted for. Never impose a minimum larger than the available screen.
    property real availableWidth: 1280
    property real availableHeight: 720
    property real frameWidth: 0
    property real frameHeight: 0
    readonly property real gutter: Math.min(24, availableWidth * 0.04, availableHeight * 0.04)
    readonly property int contentWidth: Math.max(1, Math.floor(Math.min(1080, availableWidth * 0.86, availableWidth - 2 * gutter) - frameWidth))
    readonly property int contentHeight: Math.max(1, Math.floor(Math.min(760, availableHeight * 0.82, availableHeight - 2 * gutter) - frameHeight))
}
