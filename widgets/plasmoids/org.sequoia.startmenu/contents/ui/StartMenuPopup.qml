// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore

PlasmaCore.Dialog {
    id: popup
    objectName: "startMenuWindow"
    required property var configuration
    required property var backend
    property real popupWidth: configuration.menuWidth
    property real popupHeight: configuration.menuHeight
    property int panelLocation: PlasmaCore.Types.BottomEdge
    property bool opened: false
    property bool hadFocus: false
    signal settingsRequested()
    // An edge submits Plasma's native slide hint. Floating explicitly disables it.
    // KWin transforms the complete surface and owns blur/contrast cleanup,
    // including interrupted transitions; never animate just mainItem.opacity.
    location: configuration.animationsEnabled && Kirigami.Units.longDuration > 1
        ? panelLocation : PlasmaCore.Types.Floating
    // Keep every themed corner and its matching blur mask, including the
    // taskbar-facing edge. The separate location still supplies the slide hint.
    floating: 1
    type: PlasmaCore.Dialog.PopupMenu
    hideOnWindowDeactivate: false
    backgroundHints: PlasmaCore.Dialog.StandardBackground
    // Unmap immediately on close. KWin retains the surface only for its exit
    // animation, then releases its blur and shadow with the animation reference.
    visible: opened
    onOpenedChanged: {
        if (opened) {
            Qt.callLater(function() { if (popup.opened) popup.requestActivate(); });
        }
    }
    onActiveChanged: {
        if (active) hadFocus = true;
        else if (visible && hadFocus && !menu.submenuOpen) opened = false;
    }
    onVisibleChanged: if (!visible) {
        hadFocus = false;
        opened = false;
    }
    mainItem: StartMenu {
        id: menu
        configuration: popup.configuration
        backend: popup.backend
        opened: popup.opened
        width: Math.max(1, popup.popupWidth - popup.margins.left - popup.margins.right)
        height: Math.max(1, popup.popupHeight - popup.margins.top - popup.margins.bottom)
        onDismissRequested: popup.opened = false
        onSettingsRequested: {
            popup.opened = false;
            popup.settingsRequested();
        }
    }
}
