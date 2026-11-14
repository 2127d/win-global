// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window

// Observe the owning content, not the menu's overlay (which stays visible when
// a tray switches applets). Do not watch focus: native menus can take focus.
QtObject {
    id: lifetime
    required property var popup
    property Item owner: null
    property var dismissNative: null
    readonly property var ownerWindow: owner ? owner.Window.window : null
    readonly property bool ownerVisible: owner ? owner.visible : false
    property var scope: typeof sequoiaPopupScope !== "undefined" ? sequoiaPopupScope
        : typeof mainWindow !== "undefined" ? mainWindow : null

    function dismiss() {
        if (!popup) return;
        // A hidden window cannot finish a popup's exit animation.
        if (typeof popup.exit !== "undefined") {
            const transition = popup.exit;
            popup.exit = null;
            popup.close();
            popup.exit = transition;
        } else if (typeof popup.close === "function") {
            popup.close();
        } else if (popup.visible && dismissNative) {
            dismissNative();
        }
    }
    onOwnerVisibleChanged: if (!ownerVisible) dismiss()
    property Connections windowConnection: Connections {
        target: lifetime.ownerWindow
        function onVisibleChanged() {
            if (!lifetime.ownerWindow.visible) lifetime.dismiss();
        }
    }
    property Connections scopeConnection: Connections {
        target: lifetime.scope
        function onExpandedChanged() {
            if (!lifetime.scope.expanded) lifetime.dismiss();
        }
    }
}
