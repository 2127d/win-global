// SPDX-License-Identifier: MIT
// Fake action model; no operation is ever sent to Solid or a real drive.
import QtQuick
import plasma.applet.org.kde.plasma.devicenotifier as Native
import ".."
Item {
    id: fixture
    width: 418; height: card.implicitHeight
    property string lastAction: ""
    property int feedbackCalls: 0
    function showOperationFeedback(success) { feedbackCalls++; }
    ListModel {
        id: actions
        property string defaultActionText: "Safely remove"
        property string defaultActionIcon: "media-eject"
        property string defaultActionName: "native-remove"
        ListElement { Name: "native-open"; Text: "Open in File Manager"; Icon: "system-file-manager" }
        ListElement { Name: "native-check"; Text: "Check for errors"; Icon: "tools-check-spelling" }
        function actionTriggered(name) { fixture.lastAction=name; }
    }
    DeviceItem {
        id: card
        width: parent.width; index: 0; appletInterface: fixture
        deviceDescription: "Test drive"; deviceType: "Storage Volume"; deviceIcon: "drive-removable-media"
        deviceFreeSpace: 0; deviceSize: 1024; deviceFreeSpaceText: "0 B"; deviceSizeText: "1 KiB"
        deviceIsBusy: false; deviceMounted: true; deviceState: Native.StateInfo.Idle
        deviceOperationResult: 0; deviceMessage: ""; deviceActions: actions; deviceEmblems: []
    }
    function check(value, message) { if (!value) throw new Error(message); }
    function controller() { return card.children.find(child => child.objectName === "diskDeviceCard").controller; }
    function runChecks() {
        const c=controller();
        check(c.capacityVisible && c.lowSpace && c.usedFraction === 1,"Zero free space must remain visible");
        c.triggerDefault(); check(lastAction === "native-remove","Default action identifier must be native");
        c.triggerAction("native-check"); check(lastAction === "native-check","Additional actions must retain native identifiers");
        card.deviceIsBusy=true; card.deviceState=Native.StateInfo.Unmounting;
        c.triggerDefault();check(lastAction === "native-check","Busy device must not receive default action");
        c.triggerAction("native-open");check(lastAction === "native-check","Busy device must not receive secondary action");
        check(c.status.includes("removing"),"Initial unmount status");
        return true;
    }
    function finishChecks() {
        const c=controller();
        check(c.status.includes("Don’t unplug"),"Long unmount must preserve transfer warning");
        card.deviceIsBusy=false;card.deviceState=Native.StateInfo.NotPresent;
        c.triggerDefault();check(lastAction === "native-check","Missing device must not receive action");
        card.deviceState=Native.StateInfo.Checking;card.deviceIsBusy=true;
        check(c.status.includes("Checking"),"Filesystem check state");
        card.deviceState=Native.StateInfo.Repairing;
        check(c.status.includes("Repairing"),"Filesystem repair state");
        card.deviceOperationResult=1;card.deviceMessage="The device is in use";card.deviceIsBusy=false;
        check(c.failed && c.status === "The device is in use","Native failure message must be preserved");
        card.deviceMounted=false;check(!c.capacityVisible,"Unmounted drive must not show stale capacity");
        return true;
    }
}
