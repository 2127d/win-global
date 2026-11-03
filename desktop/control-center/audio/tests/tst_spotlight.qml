// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Audio

Item {
    id: stage
    width: 418; height: 560
    Audio.Preview { id: preview; anchors.fill: parent }
    TestCase {
        name: "AudioSpotlight"
        when: windowShown
        function findIn(root,name) {
            if(root.objectName===name)return root;
            for(const child of root.children || []) {const found=findIn(child,name);if(found)return found;}
            return null;
        }
        function find(name) {const item=findIn(preview,name);verify(!!item,name);return item;}
        function init() {stage.width=418;stage.height=560;preview.fixture.reset();preview.view.applicationsExpanded=false;find("audioDeviceScroll").contentItem.contentY=0;wait(60);}
        function test_themed_device_options_dispatch_and_close() {
            mouseClick(find("audioOptions:headphones"));wait(80);
            const menu=findChild(preview,"audioDeviceOptionsMenu");verify(!!menu);verify(menu.opened);
            compare(menu.background.radius,24);
            const choice=findChild(preview,"audioOption:profile:headset");verify(!!choice);
            mouseClick(choice);wait(80);
            compare(preview.fixture.lastAction,"option:headphones:headset");
            verify(!menu.visible);
            mouseClick(find("audioOptions:headphones"));wait(80);
            keyClick(Qt.Key_Escape);wait(180);verify(!menu.visible);
            mouseClick(find("audioOptions:headphones"));wait(80);
            preview.fixture.outputs=[];wait(180);verify(!menu.visible,"Removing a device closes its menu");
        }
        function test_layout_picker_and_shared_state() {
            mouseClick(find("audioLayoutPicker")); wait(30);
            const channels = findChild(preview,"audioLayoutChannels");verify(!!channels);
            mouseClick(channels);wait(40);
            compare(preview.fixture.layoutMode,"channels");
            verify(!!find("audioOutputChannel"));verify(!!find("audioInputChannel"));
            const vertical=find("audioSlider:headphones");compare(vertical.orientation,Qt.Vertical);
            const oldValue=preview.fixture.outputs[0].volume;
            mouseClick(vertical,vertical.width/2,vertical.height*0.2);wait(25);
            verify(preview.fixture.outputs[0].volume>oldValue);
            const changed=preview.fixture.outputs[0].volume;
            preview.fixture.setLayout("mixer");wait(40);
            verify(!!find("audioMixerOutput"));verify(!!find("audioMixerApp:browser"));
            compare(find("audioSlider:headphones").value,changed);
            preview.fixture.setLayout("spotlight");wait(30);
            compare(find("audioSlider:headphones").value,changed);
            verify(!!find("audioOutputCard"));
        }
        function test_app_slider_survives_live_updates_while_dragging() {
            for(const mode of ["spotlight","channels","mixer"]) {
                preview.fixture.setLayout(mode);preview.view.applicationsExpanded=true;wait(30);
                const scroll=find("audioDeviceScroll").contentItem;
                scroll.contentY=Math.max(0,scroll.contentHeight-scroll.height);wait(30);
                const slider=find("audioSlider:chat");
                mousePress(slider,slider.width*0.3,slider.height/2);wait(15);
                mouseMove(slider,slider.width*0.6,slider.height/2);wait(15);
                verify(slider===find("audioSlider:chat"),"Volume updates must retain the dragged control");
                mouseMove(slider,slider.width*0.8,slider.height/2);wait(15);
                mouseRelease(slider,slider.width*0.8,slider.height/2);wait(20);
                verify(preview.fixture.playback[1].volume>70);
            }
        }
        function test_mixer_routes_stream_without_changing_default() {
            preview.fixture.setLayout("mixer");wait(40);
            const route=find("audioRoute:browser");
            compare(route.displayText,"Heavys H1H");
            route.activated(1);wait(30);
            compare(preview.fixture.playback[0].deviceIndex,2);
            compare(preview.fixture.outputId,"headphones");
            compare(route.displayText,"Built-in speakers");
            preview.fixture.outputs=preview.fixture.outputs.filter(d=>d.id!=="speakers");wait(30);
            compare(route.displayText,"Device unavailable");
            preview.fixture.recording=[{id:"recorder",type:"source-output",name:"Recorder",deviceIndex:3,volume:50,muted:false,hasVolume:true,writable:true}];wait(30);
            compare(find("audioRoute:recorder").displayText,"Built-in microphone");
            preview.fixture.outputs=[];preview.fixture.inputs=[];wait(30);
            compare(preview.view.output,null);compare(preview.view.input,null);
        }
        function test_all_layouts_fit_narrow_and_empty_states() {
            for(const mode of ["spotlight","channels","mixer"]) {
                preview.fixture.setLayout(mode);
                for(const width of [292,418,552]) {
                    stage.width=width;wait(35);
                    const names=mode==="channels" ? ["audioOutputChannel","audioInputChannel"] : mode==="mixer" ? ["audioMixerOutput","audioMixerApp:browser"] : ["audioOutputCard","audioInputCard"];
                    for(const name of names) {
                        const card=find(name);const pos=card.mapToItem(preview,0,0);
                        verify(card.width>90);verify(pos.x>=0 && pos.x+card.width<=width, name+" must fit "+width);
                    }
                    verify(find("audioActionIsland").width<width);
                }
            }
            preview.fixture.outputs=[];preview.fixture.inputs=[];preview.fixture.playback=[];
            for(const mode of ["spotlight","channels","mixer"]) {preview.fixture.setLayout(mode);wait(30);compare(preview.view.output,null);compare(preview.view.streams.length,0);}
        }
        function test_volume_mute_external_updates_and_limits() {
            const slider=find("audioSlider:headphones");
            mouseClick(slider,slider.width*0.4,slider.height/2);wait(25);
            compare(preview.fixture.lastAction,"volume:headphones");
            verify(preview.fixture.outputs[0].volume>30 && preview.fixture.outputs[0].volume<50);
            mouseClick(find("audioMute:headphones"));wait(25);
            verify(preview.fixture.outputs[0].muted);
            compare(find("audioPercent:headphones").text,"Muted");
            preview.fixture.change("headphones",{volume:27,muted:false});wait(25);
            compare(slider.value,27);
            preview.fixture.maximumVolume=150;wait(25);compare(slider.to,150);
            preview.fixture.change("headphones",{volume:120});preview.fixture.maximumVolume=100;wait(25);
            compare(slider.to,120,"Opening controls must not silently lower an amplified device");
            preview.fixture.change("headphones",{writable:false});wait(25);verify(!slider.enabled);
        }
        function test_select_output_and_handle_hotplug() {
            const picker=find("audioOutputPicker");
            compare(picker.displayText,"Heavys H1H");
            mouseClick(picker);wait(60);
            const selected=picker.popup.contentItem.itemAtIndex(0);
            verify(selected.visible,"The selected device must remain visible in the dropdown");
            verify(selected.checked);
            compare(picker.popup.background.radius,24);
            mouseClick(picker.popup.contentItem.itemAtIndex(1));wait(50);
            compare(preview.fixture.outputId,"speakers");
            compare(preview.view.output.name,"Built-in speakers");
            preview.fixture.outputs=preview.fixture.outputs.filter(d=>d.id!=="speakers");wait(30);
            compare(preview.view.output.id,"headphones");
            compare(find("audioOutputPicker").displayText,"Heavys H1H");
            verify(!find("audioOutputPicker").enabled);
        }
        function test_apps_recording_streams_and_microphone() {
            preview.view.applicationsExpanded=true;wait(40);
            verify(!!find("audioApp:browser"));
            const scroll=find("audioDeviceScroll").contentItem;
            scroll.contentY=Math.max(0,scroll.contentHeight-scroll.height);wait(30);
            mouseClick(find("audioMute:chat"));wait(20);compare(preview.fixture.lastAction,"mute:chat");
            verify(preview.fixture.playback[1].muted);
            preview.fixture.recording=[{id:"recorder",type:"source-output",name:"Recorder",volume:55,muted:false,hasVolume:false,writable:false}];wait(40);
            verify(!!find("audioApp:recorder"));verify(!find("audioSlider:recorder").visible);
            preview.fixture.playback=[];preview.fixture.recording=[];wait(30);
            compare(preview.view.streams.length,0);
            scroll.contentY=0;wait(30);mouseClick(find("audioMute:microphone"));wait(25);
            compare(preview.fixture.lastAction,"mute:microphone");verify(preview.fixture.inputs[0].muted);
        }
        function test_settings_empty_and_footer() {
            const island=find("audioActionIsland");const y=island.mapToItem(preview,0,0).y;
            mouseClick(find("audioSettings"));compare(preview.fixture.lastAction,"settings");
            mouseClick(find("audioMore"));compare(preview.fixture.lastAction,"more");
            preview.fixture.outputs=[];preview.fixture.inputs=[];preview.fixture.ready=false;wait(40);
            compare(preview.view.output,null);compare(preview.view.input,null);
            compare(island.mapToItem(preview,0,0).y,y);
            preview.fixture.settingsAllowed=false;verify(!find("audioSettings").enabled);
            preview.fixture.settingsAllowed=true;
        }
        function test_narrow_long_names_and_input_selection() {
            preview.fixture.change("headphones",{name:"Wireless headphones with a very long device description"});
            preview.fixture.inputs=preview.fixture.inputs.concat([{id:"usbmic",type:"source",name:"USB microphone",subtitle:"",volume:50,muted:false,hasVolume:true,writable:true}]);wait(30);
            for(const width of [292,418,552]) {
                stage.width=width;wait(30);
                for(const name of ["audioOutputCard","audioInputCard","audioActionIsland"]) {
                    const card=find(name);verify(card.width>160 && card.width<width);
                }
                verify(find("audioInputPicker").visible);
            }
            find("audioInputPicker").activated(1);wait(30);
            compare(preview.view.input.id,"usbmic");
        }
    }
}
