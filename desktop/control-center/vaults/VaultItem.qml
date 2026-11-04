// SPDX-License-Identifier: MIT
import QtQuick
Item {
    id: root
    required property int index
    required property var model
    required property var actions
    implicitHeight: card.implicitHeight
    QtObject {
        id: nativeController
        readonly property string name: root.model.name
        readonly property string icon: root.model.icon
        readonly property bool opened: root.model.isOpened
        readonly property bool offlineOnly: root.model.isOfflineOnly
        readonly property string message: root.model.message
        readonly property bool available: root.model.isEnabled
        readonly property bool busy: root.actions.isBusy
        readonly property bool canAct: available && !busy
        readonly property string emblem: message.length > 0 ? "emblem-error" : opened ? "emblem-encrypted-unlocked" : offlineOnly ? "emblem-unmounted" : ""
        function primary() {
            if (!canAct) return;
            if (opened) root.actions.toggle(root.model.device);
            else root.actions.openInFileManager(root.model.device);
        }
        function secondary() {
            if (!canAct) return;
            if (opened) root.actions.openInFileManager(root.model.device);
            else root.actions.toggle(root.model.device);
        }
        function configure() { if (canAct) root.actions.configure(root.model.device); }
        function forceLock() { if (canAct && opened) root.actions.forceClose(root.model.device); }
    }
    VaultCard { id: card; width: parent.width; controller: nativeController }
}
