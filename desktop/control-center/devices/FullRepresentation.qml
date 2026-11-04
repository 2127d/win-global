// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kcmutils

PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaDetailPage"
    required property var appletInterface
    property var model
    property bool spontaneousOpen: false
    readonly property real sequoiaPreferredWidth: 418
    readonly property real sequoiaPreferredHeight: view.naturalHeight
    padding: 0; background: null
    Timer {
        id: messageAttention; interval: 6000
        onRunningChanged: root.appletInterface.isMessageHighlightAnimatorRunning = running
    }
    QtObject {
        id: nativeController
        readonly property var devicesModel: root.model
        readonly property string filter: Plasmoid.configuration.allDevices ? "all" : Plasmoid.configuration.removableDevices ? "removable" : "internal"
        readonly property int unmountableCount: root.model.unmountableCount
        readonly property bool settingsAllowed: root.appletInterface.openAutomounterKcmAuthorized
        readonly property bool autoPopup: Plasmoid.configuration.popupOnNewDevice
        function setFilter(filter) {
            if (!["removable","internal","all"].includes(filter)) return;
            Plasmoid.configuration.removableDevices = filter === "removable";
            Plasmoid.configuration.nonRemovableDevices = filter === "internal";
            Plasmoid.configuration.allDevices = filter === "all";
        }
        function settings() { if (settingsAllowed) KCMLauncher.openSystemSettings("kcm_device_automounter"); }
        function removeAll() { if (unmountableCount > 0) root.model.unmountAllRemovables(); }
        function setAutoPopup(enabled) { Plasmoid.configuration.popupOnNewDevice = enabled; }
    }
    DevicesView {
        id: view; anchors.fill: parent; controller: nativeController
        deviceDelegate: Component {
            DeviceItem {
                Layout.fillWidth: true
                appletInterface: root.appletInterface
                onMessageShown: messageAttention.restart()
            }
        }
    }
}
