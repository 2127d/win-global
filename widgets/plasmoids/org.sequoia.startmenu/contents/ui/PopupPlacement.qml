// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import org.kde.plasma.core as PlasmaCore

QtObject {
    id: placement
    required property Item anchor
    required property rect screenRect
    required property int location
    required property int requestedWidth
    required property int requestedHeight
    readonly property int panelGap: 3
    readonly property int screenMargin: 12
    readonly property var panelWindow: anchor.Window.window
    readonly property var panelSurface: {
        if (!panelWindow) return null;
        // Plasma's panel surface exposes its painted offset separately from
        // the transparent floating/shadow margins of the containing window.
        for (const child of panelWindow.contentItem.children) {
            if (child.floatingTranslucentItemOffset !== undefined) return child;
        }
        return null;
    }
    readonly property rect panelRect: {
        if (!panelWindow) return Qt.rect(screenRect.x, screenRect.y + screenRect.height, screenRect.width, 0);
        if (panelSurface && panelWindow.thickness !== undefined) {
            const offset = panelSurface.floatingTranslucentItemOffset;
            const origin = panelSurface.mapToItem(panelWindow.contentItem, offset.x, offset.y);
            const vertical = location === PlasmaCore.Types.LeftEdge || location === PlasmaCore.Types.RightEdge;
            return Qt.rect(panelWindow.x + origin.x, panelWindow.y + origin.y,
                vertical ? panelWindow.thickness : panelSurface.width - panelSurface.leftFloatingPadding - panelSurface.rightFloatingPadding,
                vertical ? panelSurface.height - panelSurface.topFloatingPadding - panelSurface.bottomFloatingPadding : panelWindow.thickness);
        }
        return Qt.rect(panelWindow.x, panelWindow.y, panelWindow.width, panelWindow.height);
    }
    readonly property bool topPanel: location === PlasmaCore.Types.TopEdge
    readonly property bool bottomPanel: location === PlasmaCore.Types.BottomEdge
    readonly property bool leftPanel: location === PlasmaCore.Types.LeftEdge
    readonly property bool rightPanel: location === PlasmaCore.Types.RightEdge
    readonly property real usableHeight: topPanel
        ? screenRect.y + screenRect.height - screenMargin - panelRect.y - panelRect.height - panelGap
        : bottomPanel ? panelRect.y - panelGap - screenRect.y - screenMargin
        : screenRect.height - screenMargin * 2
    readonly property real usableWidth: leftPanel
        ? screenRect.x + screenRect.width - screenMargin - panelRect.x - panelRect.width - panelGap
        : rightPanel ? panelRect.x - panelGap - screenRect.x - screenMargin
        : screenRect.width - screenMargin * 2
    readonly property int popupWidth: Math.max(1, Math.min(requestedWidth, usableWidth))
    readonly property int popupHeight: Math.max(1, Math.min(requestedHeight, usableHeight))
    readonly property int popupX: Math.round(leftPanel ? panelRect.x + panelRect.width + panelGap
        : rightPanel ? panelRect.x - panelGap - popupWidth
        : screenRect.x + (screenRect.width - popupWidth) / 2)
    readonly property int popupY: Math.round(topPanel ? panelRect.y + panelRect.height + panelGap
        : bottomPanel ? panelRect.y - panelGap - popupHeight
        : screenRect.y + (screenRect.height - popupHeight) / 2)
}
