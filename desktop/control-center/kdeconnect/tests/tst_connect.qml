// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."

Item {
    id: host
    width: 760; height: 1000
TestCase {
    id: tests
    name: "KDEConnect"
    width: 760; height: 1000
    when: windowShown
    Component { id: previewComponent; Preview {} }
    property var page
    function item(name) { const found=findChild(page,name); verify(found !== null,"Missing " + name); return found; }
    function init() { page=createTemporaryObject(previewComponent,host); verify(page); wait(80); }
    function test_actions() {
        for (const id of ["share","browse","clipboard","sms","ring","display"]) {
            mouseClick(item("connectAction:"+id)); compare(page.fixture.lastAction,id);
        }
        page.fixture.clipboardAvailable=false; wait(30);
        compare(findChild(page,"connectAction:clipboard"),null);
        page.fixture.virtualActive=true; tryCompare(item("connectAction:display"),"checked",true);
        page.fixture.reachable=false; wait(20);
        for (const id of ["share","browse","sms","ring","display"]) compare(item("connectAction:"+id).enabled,false);
    }
    function test_widths_and_sizing() {
        const heading=item("connectHeading");
        for (const name of ["connectDeviceCard","connectNotificationsCard","connectCommandsCard"]) {
            const card=item(name); compare(Math.round(card.width),Math.round(heading.width));
            compare(Math.round(card.mapToItem(page,0,0).x),Math.round(heading.x));
        }
        verify(item("connectFooter").width < heading.width);
        const small=page.view.naturalHeight;
        item("connectDeviceView").notificationsExpanded=true; wait(50);
        verify(page.view.naturalHeight > small+60);
        item("connectDeviceView").notificationsExpanded=false; wait(50);
        fuzzyCompare(page.view.naturalHeight,small,1);
        page.fixture.deviceCount=2; wait(80); verify(page.view.naturalHeight > small+100);
    }
    function test_notification_reply() {
        item("connectDeviceView").notificationsExpanded=true; wait(50);
        mouseClick(item("connectReply")); verify(item("connectNotification").replying);
        const input=item("connectReplyText");
        compare(item("connectSendReply").enabled,false);
        keyClick(Qt.Key_Return); compare(page.fixture.replyCalls,0);
        input.text="First line"; input.forceActiveFocus(); keyClick(Qt.Key_Return,Qt.ShiftModifier);
        verify(input.text.includes("\n")); compare(page.fixture.replyCalls,0);
        input.text="See you there"; keyClick(Qt.Key_Return);
        compare(page.fixture.lastReply,"See you there"); compare(page.fixture.replyCalls,1);
        compare(item("connectNotification").replying,false);
        mouseClick(item("connectReply")); input.text="discard"; keyClick(Qt.Key_Escape);
        compare(input.text,""); compare(item("connectNotification").replying,false);
        mouseClick(item("connectDismiss")); compare(page.fixture.dismissCalls,1);
        mouseClick(item("connectDismissAll")); compare(page.fixture.dismissAllCalls,1);
    }
    function test_commands() {
        const device=item("connectDeviceView");
        mouseClick(item("connectCommandsToggle")); verify(device.commandsExpanded); wait(50);
        mouseClick(item("connectCommand:presentation")); compare(page.fixture.lastCommand,"presentation");
        mouseClick(item("connectAddCommand")); compare(page.fixture.addCalls,1);
        page.fixture.commandModel=[]; wait(30);
        verify(item("connectCommandsCard").visible); verify(item("connectAddCommand").visible);
        mouseClick(item("connectAddCommand")); compare(page.fixture.addCalls,2);
        page.fixture.commandsAvailable=false; wait(30); compare(item("connectCommandsCard").visible,false);
    }
    function test_zero_battery_and_empty() {
        page.fixture.batteryText="0%"; compare(item("connectBattery").text,"0%"); verify(item("connectBattery").visible);
        page.fixture.deviceCount=0; page.fixture.pairedCount=0; page.fixture.discoveredCount=0; wait(50);
        verify(item("connectEmptyCard").visible); verify(item("connectPairDevice").visible);
        mouseClick(item("connectPairDevice")); compare(page.fixture.settingsCalls,1);
        mouseClick(item("connectSettings")); compare(page.fixture.settingsCalls,2);
        page.fixture.pairedCount=1; wait(20); compare(item("connectPairDevice").visible,false);
    }
}

}
