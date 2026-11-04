// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Bluetooth
import "../BluetoothDevices.js" as Devices

Item {
    id: stage
    width: 418; height: 520
    Bluetooth.Preview { id: preview; anchors.fill: parent }
    TestCase {
        name: "BluetoothSpotlight"
        when: windowShown
        function findIn(root, name) {
            if (root.objectName === name) return root;
            for (const child of root.children || []) { const result = findIn(child,name); if (result) return result; }
            return null;
        }
        function find(name) { const item = findIn(preview,name); verify(!!item, name); return item; }
        function init() { stage.width=418; stage.height=520; preview.view.selectionState = {order:[],activeIds:[],featuredId:"",manualId:""}; preview.fixture.reset(); preview.view.goBack(); wait(50); tryCompare(preview.view,"navigationBusy",false); }
        function test_connection_order_and_battery_values() {
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("headphones",{battery:0}); wait(30);
            verify(find("bluetoothBattery:headphones").text.includes("0% battery"));
            preview.fixture.change("headphones",{battery:-1}); wait(20);
            verify(!find("bluetoothBattery:headphones").text.includes("%"));
            preview.fixture.change("headphones",{connected:false}); wait(30);
            compare(preview.view.featured.id,"mouse");
            preview.fixture.change("headphones",{connected:true}); wait(30);
            compare(preview.view.featured.id,"mouse", "Reconnecting does not jump the queue");
            preview.fixture.change("keyboard",{connected:true}); wait(30);
            preview.fixture.change("mouse",{connected:false}); wait(30);
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("headphones",{connected:false}); wait(30);
            compare(preview.view.featured.id,"keyboard");
            compare(Devices.battery(null),-1);compare(Devices.battery(101),-1);compare(Devices.battery(0),0);
        }
        function test_activity_switcher_and_manual_selection() {
            preview.fixture.change("keyboard",{connected:true,name:"Phone",icon:"smartphone-symbolic"}); wait(30);
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("headphones",{playing:true}); wait(30);
            preview.fixture.change("keyboard",{playing:true}); wait(30);
            compare(preview.view.featured.id,"keyboard", "New playback takes focus even while headphones play");
            mouseClick(find("bluetoothSwitch:headphones")); wait(30);
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("keyboard",{battery:40}); wait(30);
            compare(preview.view.featured.id,"headphones", "Unrelated updates preserve the manual choice");
            preview.fixture.change("keyboard",{playing:false}); wait(30);
            preview.fixture.change("keyboard",{transferring:true}); wait(30);
            compare(preview.view.featured.id,"keyboard");
            compare(find("bluetoothDevice:keyboard").status,"Transferring files");
            preview.fixture.change("keyboard",{transferring:false}); wait(30);
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("headphones",{playing:false}); wait(30);
            compare(preview.view.featured.id,"headphones");
            preview.fixture.change("headphones",{connected:false}); wait(30);
            compare(preview.view.featured.id,"mouse");
        }
        function test_idle_devices_leave_switcher_and_release_manual_choice() {
            compare(preview.view.switchDevices.length,0,"Idle connections have no activity icons");
            preview.fixture.change("headphones",{playing:true});
            preview.fixture.change("keyboard",{connected:true,transferring:true}); wait(30);
            compare(preview.view.switchDevices.length,2);
            mouseClick(find("bluetoothSwitch:keyboard")); wait(20);
            compare(preview.view.selectionState.manualId,"keyboard");
            preview.fixture.change("keyboard",{transferring:false}); wait(30);
            compare(preview.view.featured.id,"headphones");
            compare(preview.view.selectionState.manualId,"");
            compare(preview.view.switchDevices.length,1);
            verify(!findIn(preview,"bluetoothSwitch:keyboard"));
            verify(find("bluetoothSwitch:headphones").visible);
            verify(find("bluetoothDevice:keyboard").visible,"Idle phone remains in the device list");
            preview.fixture.change("headphones",{playing:false}); wait(30);
            compare(preview.view.switchDevices.length,0);
            verify(!findIn(preview,"bluetoothSwitch:headphones"));
            compare(preview.view.featured.id,"headphones","No activity returns to the first connection");
        }
        function test_native_connection_order_and_activity_mapping() {
            const devices = [{id:"phone",connected:true},{id:"headphones",connected:true}];
            compare(Devices.selection(devices,{},["headphones","phone"]).featuredId,"headphones");
            const objects = {
                "/headphones/sep/fd0": {"org.bluez.MediaTransport1":{State:"active",Device:"/headphones"}},
                "/phone/player0": {"org.bluez.MediaPlayer1":{Status:"playing"}},
                "/idle/player0": {"org.bluez.MediaPlayer1":{Status:"paused"}}
            };
            const playing=Devices.playingDevices(objects);
            verify(playing["/headphones"]); verify(!playing["/phone"]); verify(!playing["/idle"]);
            objects["/headphones/sep/fd0"]["org.bluez.MediaTransport1"].State = "idle";
            compare(Object.keys(Devices.playingDevices(objects)).length,0,"Player status alone cannot keep a device active");
            const transfers = {
                "/session": {"org.bluez.obex.Session1":{Destination:"aa:bb"}},
                "/session/transfer0": {"org.bluez.obex.Transfer1":{Status:"active",Session:"/session"}}
            };
            verify(Devices.transferringDevices(transfers)["AA:BB"]);
            transfers["/session/transfer0"]["org.bluez.obex.Transfer1"].Status="complete";
            compare(Object.keys(Devices.transferringDevices(transfers)).length,0);
            delete transfers["/session"];compare(Object.keys(Devices.transferringDevices(transfers)).length,0);
        }
        function test_native_action_routing_busy_failure_and_retry() {
            preview.fixture.change("mouse",{connected:false}); wait(30);
            mouseClick(find("bluetoothSpotlightConnect")); wait(30);
            compare(preview.fixture.lastAction,"disconnect:headphones");
            verify(!find("bluetoothSpotlightConnect").enabled);
            preview.fixture.finish("headphones",false); wait(30);
            compare(find("bluetoothSpotlightConnect").text,"Connect");
            mouseClick(find("bluetoothSpotlightConnect")); wait(30);
            compare(preview.fixture.lastAction,"connect:headphones");
            preview.fixture.finish("headphones",true); wait(30);
            verify(find("bluetoothSpotlightConnect").enabled);
            verify(find("bluetoothDevice:headphones").status.indexOf("failed")>=0);
            mouseClick(find("bluetoothSpotlightConnect")); wait(30);
            preview.fixture.finish("headphones",false); wait(30);
            compare(find("bluetoothSpotlightConnect").text,"Disconnect");
        }
        function test_details_back_forget_and_removed_device() {
            mouseClick(find("bluetoothSpotlightDetails")); tryCompare(preview.view,"navigationBusy",false);
            compare(preview.view.navigationDepth,2);
            const scroll=find("bluetoothDeviceScroll").contentItem;
            scroll.contentY=Math.max(0,scroll.contentHeight-scroll.height);wait(30);
            mouseClick(find("bluetoothForget"));compare(preview.fixture.lastAction,"forget:headphones");
            compare(preview.fixture.devices.length,3,"forget routes to native confirmation, without deleting directly");
            preview.view.forceActiveFocus();keyClick(Qt.Key_Escape);tryCompare(preview.view,"navigationBusy",false);
            compare(preview.view.navigationDepth,1);
            preview.view.openDetails("mouse");wait(20);
            preview.fixture.devices=preview.fixture.devices.filter(d=>d.id!=="mouse");wait(40);
            compare(preview.view.navigationDepth,1);
        }
        function test_power_empty_adapter_and_fixed_footer() {
            const island=find("bluetoothActionIsland");const y=island.mapToItem(preview,0,0).y;
            mouseClick(find("bluetoothPair"));compare(preview.fixture.lastAction,"pair");
            mouseClick(find("bluetoothSettings"));compare(preview.fixture.lastAction,"settings");
            preview.view.openDetails("headphones");wait(30);
            mouseClick(find("bluetoothHeaderSwitch"));wait(30);
            verify(!preview.fixture.operational);compare(preview.view.navigationDepth,1);
            tryCompare(preview.view,"navigationBusy",false);
            verify(!find("bluetoothPair").enabled);
            mouseClick(find("bluetoothEmptyAction"));wait(30);verify(preview.fixture.operational);
            preview.fixture.devices=[];wait(30);compare(find("bluetoothEmptyAction").text,"Pair new device");
            preview.fixture.adapterAvailable=false;preview.fixture.operational=false;wait(30);
            verify(!find("bluetoothEmptyAction").visible);
            compare(island.mapToItem(preview,0,0).y,y);
        }
        function test_narrow_layout_and_long_names() {
            preview.fixture.change("headphones",{name:"A very long pair of wireless headphones with a long device name"});
            for (const width of [292,418,552]) {
                stage.width=width;wait(40);
                for (const d of preview.fixture.devices) {
                    const card=find("bluetoothDevice:"+d.id);
                    verify(card.width>180 && card.width<width);
                }
                const island=find("bluetoothActionIsland");verify(island.width<width);
            }
        }
    }
}
