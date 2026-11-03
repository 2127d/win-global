// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import org.kde.plasma.networkmanagement as PlasmaNM
import ".." as Network
Item {
    id:stage;width:418;height:680
    Network.Preview{id:preview;anchors.fill:parent}
    Network.NetworkRadio{id:radio;visible:false;width:200;title:"Wi-Fi";iconName:"network-wireless-symbolic";checked:false;onToggled:checked=!checked}
    TestCase {
        name:"NetworkLayouts";when:windowShown
        function search(root,name){if(root.objectName===name && root.visible)return root;for(const child of root.children || []){const result=search(child,name);if(result)return result;}return null;}
        function find(name){const item=search(preview,name);verify(!!item,name);return item;}
        function init(){radio.visible=false;radio.enabled=true;radio.checked=false;stage.width=418;preview.fixture.reset();preview.summary.allNetworks=false;preview.scroll.contentItem.contentY=0;wait(40);}
        function test_radio_mouse_keyboard_and_disabled_state() {
            radio.visible=true;wait(30);const toggle=findChild(radio,"Switch");
            verify(!!toggle);mouseClick(toggle);verify(radio.checked);toggle.forceActiveFocus();keyClick(Qt.Key_Space);verify(!radio.checked);
            radio.enabled=false;mouseClick(toggle);verify(!radio.checked);radio.visible=false;
        }
        function test_grouped_available_networks_and_secondary_connection() {
            const available=Array.from({length:6},(_,i)=>({row:i+3,name:"Nearby "+i,icon:"network-wireless-symbolic",type:PlasmaNM.Enums.Wireless,active:false,connecting:false,secured:true,saved:i===0,signal:80-i*10}));
            preview.fixture.entries=preview.fixture.entries.concat(available);wait(50);
            compare(preview.summary.available.length,6);compare(preview.summary.secondary.length,1);
            verify(!!find("networkSecondary:1"));verify(!search(preview,"networkAvailable:0"));verify(!search(preview,"networkAvailable:1"));
            verify(!!find("networkAvailable:3"));verify(!search(preview,"networkAvailable:8"));
            const flick=preview.scroll.contentItem;flick.contentY=Math.max(0,flick.contentHeight-flick.height);wait(50);
            mouseClick(find("networkShowAll"));wait(40);verify(preview.summary.allNetworks);verify(!!find("networkAvailable:8"));
            flick.contentY=Math.max(0,flick.contentHeight-flick.height);wait(50);mouseClick(find("networkAvailable:8"));compare(preview.fixture.lastAction,"details:8");
        }
        function test_switch_all_layouts_and_checkmarks(){
            for(const choice of [{id:"networkLayoutDesk",mode:"desk"},{id:"networkLayoutCompact",mode:"compact"},{id:"networkLayoutSpotlight",mode:"spotlight"}]){
                mouseClick(find("networkLayoutButton"));const menu=findChild(preview,"networkLayoutMenu");verify(!!menu);tryCompare(menu,"opened",true,1000,choice.mode);const entry=findChild(preview,choice.id);verify(!!entry);mouseClick(entry);wait(40);compare(preview.fixture.layoutMode,choice.mode);verify(entry.checked);tryCompare(menu,"visible",false,1000);
            }
        }
        function test_hotspot_and_qr_buttons_respect_availability(){
            mouseClick(find("networkHotspot"));compare(preview.fixture.lastAction,"hotspot");verify(preview.fixture.hotspotActive);
            preview.fixture.hotspotSupported=false;wait(20);verify(!find("networkHotspot").enabled);mouseClick(find("networkHotspot"));verify(preview.fixture.hotspotActive);
            mouseClick(find("networkScanQr"));compare(preview.fixture.lastAction,"qr");preview.fixture.qrAvailable=false;wait(20);verify(!find("networkScanQr").enabled);
            preview.fixture.lastAction="";mouseClick(find("networkScanQr"));compare(preview.fixture.lastAction,"");
            mouseClick(find("networkSettings"));compare(preview.fixture.lastAction,"settings");
        }
        function test_spotlight_tracks_disconnect_and_connecting(){
            compare(preview.summary.primary.name,"Home network");mouseClick(find("networkPrimaryDetails"));compare(preview.fixture.lastAction,"details:0");
            preview.fixture.entries=preview.fixture.entries.slice(1);wait(20);compare(preview.summary.primary.name,"Ethernet");
            preview.fixture.entries=[{row:5,name:"Connecting",icon:"network-wireless-symbolic",type:PlasmaNM.Enums.Wireless,active:false,connecting:true}];wait(20);verify(preview.summary.primary.connecting);
            preview.fixture.entries=[];wait(20);compare(preview.summary.primary,null);verify(!search(preview,"networkPrimaryDetails"));
        }
        function test_desk_cards_and_narrow_footer(){
            for(const mode of ["spotlight","desk","compact"])for(const width of [292,418,552]){
                preview.fixture.layoutMode=mode;stage.width=width;wait(30);
                const island=find("networkActionIsland");verify(island.height<=48);verify(island.width<=width);
                for(const name of ["networkSettings","networkHotspot","networkScanQr","networkLayoutButton"]){const b=find(name);const p=b.mapToItem(preview,0,0);verify(p.x>=0 && p.x+b.width<=width);verify(b.width>=32);}
            }
            compare(preview.summary.wireless.name,"Home network");compare(preview.summary.wired.name,"Ethernet");
        }
    }
}
