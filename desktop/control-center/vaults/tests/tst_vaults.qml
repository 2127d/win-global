// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."
Item {
    id: host; width:700;height:1000
    Component {id:sample;Preview {}}
    Component {
        id: adapterSample
        VaultItem {
            width:418;index:0
            model: QtObject { property string device:"fixture-device";property string name:"Fixture vault";property string icon:"plasmavault";property bool isOpened:false;property bool isOfflineOnly:false;property string message:"";property bool isEnabled:true }
            actions: QtObject {
                property bool isBusy:false
                property string lastAction:""
                property string lastDevice:""
                function toggle(device) {lastAction="toggle";lastDevice=device;}
                function openInFileManager(device) {lastAction="open";lastDevice=device;}
                function configure(device) {lastAction="configure";lastDevice=device;}
                function forceClose(device) {lastAction="force";lastDevice=device;}
            }
        }
    }
    TestCase {
        name:"Vaults";when:windowShown
        property var page
        function item(name) {const found=findChild(page,name);verify(!!found,name);return found;}
        function init() {page=createTemporaryObject(sample,host);verify(page);wait(60);}
        function test_native_action_routing() {
            const adapter=createTemporaryObject(adapterSample,host);verify(adapter);wait(40);
            const card=findChild(adapter,"vaultCard");verify(card);const c=card.controller;
            c.primary();compare(adapter.actions.lastAction,"open");compare(adapter.actions.lastDevice,"fixture-device");
            c.secondary();compare(adapter.actions.lastAction,"toggle");
            adapter.model.isOpened=true;c.primary();compare(adapter.actions.lastAction,"toggle");
            c.secondary();compare(adapter.actions.lastAction,"open");c.configure();compare(adapter.actions.lastAction,"configure");
            c.forceLock();compare(adapter.actions.lastAction,"force");
            adapter.actions.lastAction="";adapter.actions.isBusy=true;
            c.primary();c.secondary();c.configure();c.forceLock();compare(adapter.actions.lastAction,"");
            adapter.actions.isBusy=false;adapter.model.isEnabled=false;c.primary();compare(adapter.actions.lastAction,"");
            adapter.model.isEnabled=true;adapter.model.isOpened=false;c.forceLock();compare(adapter.actions.lastAction,"");
        }
        function test_actions_and_states() {
            compare(item("vaultStatus").text,"Locked");verify(!item("vaultForceLock").visible);
            mouseClick(item("vaultPrimary"));compare(page.vault.primaryCalls,1);
            mouseClick(item("vaultSecondary"));compare(page.vault.secondaryCalls,1);
            mouseClick(item("vaultConfigure"));compare(page.vault.configureCalls,1);
            page.vault.opened=true;wait(20);compare(item("vaultStatus").text,"Unlocked");verify(item("vaultForceLock").visible);
            mouseClick(item("vaultForceLock"));compare(page.vault.forceCalls,1);
            page.vault.busy=true;wait(20);verify(item("vaultBusy").visible);verify(!item("vaultPrimary").enabled);
            page.vault.offlineOnly=true;verify(item("vaultOffline").visible);
        }
        function test_widths_errors_and_content_size() {
            const header=item("vaultHeading");compare(item("vaultCard").width,header.width);compare(item("vaultCard").mapToItem(page,0,0).x,header.x);
            verify(item("vaultFooter").width<header.width);
            const height=page.view.naturalHeight;
            page.vault.message="The vault is still in use. Close applications using its files and try again.";wait(30);
            verify(item("vaultError").visible);verify(page.view.naturalHeight>height);
            page.vault.message="";wait(30);fuzzyCompare(page.view.naturalHeight,height,1);
            page.fixture.vaultsModel=2;wait(30);verify(page.view.naturalHeight>height+100);
            page.fixture.vaultsModel=0;wait(30);verify(item("vaultEmptyCard").visible);compare(item("vaultEmptyCard").width,header.width);
            mouseClick(item("vaultCreate"));compare(page.fixture.createCalls,1);
        }
    }
}
