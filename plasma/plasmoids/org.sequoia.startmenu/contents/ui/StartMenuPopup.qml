// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.core as PlasmaCore

PlasmaCore.Dialog {
    id: popup
    objectName: "startMenuWindow"
    required property var configuration
    required property var backend
    property real popupWidth: configuration.menuWidth
    property real popupHeight: configuration.menuHeight
    property bool opened: false
    property bool hadFocus: false
    signal settingsRequested()
    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.PopupMenu
    hideOnWindowDeactivate: false
    backgroundHints: PlasmaCore.Dialog.StandardBackground
    // Hide the whole native surface together, including its background and shadow.
    // KWin owns window effects; a second content-only exit leaves an empty box.
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
