// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Display

Item {
    id:stage;width:418;height:680
    Display.Preview{id:preview;anchors.fill:parent}
    ListModel{id:nativeModel}
    Display.DisplayRecords{id:nativeRecords;displayModel:nativeModel}
    TestCase {
        name:"DisplaySpotlight";when:windowShown
        function search(root,name){if(root.objectName===name && root.visible)return root;for(const c of root.children || []){const r=search(c,name);if(r)return r;}return null;}
        function find(name){const r=search(preview,name);verify(!!r,name);return r;}
        function scrollBottom(){const flick=find("displayScroll").contentItem;flick.contentY=Math.max(0,flick.contentHeight-flick.height);wait(40);}
        function init(){stage.width=418;stage.height=680;preview.fixture.reset(5);preview.view.allExpanded=false;preview.view.selectedId="screen0";find("displayScroll").contentItem.contentY=0;wait(60);}
        function test_five_displays_selection_and_brightness(){
            compare(preview.view.displays.length,5);mouseClick(find("displaySelect:screen4"));wait(30);compare(preview.view.selectedId,"screen4");
            const slider=find("displaySlider:screen4");mouseClick(slider,slider.width*0.65,slider.height/2);wait(30);compare(preview.fixture.lastAction,"brightness:screen4");compare(preview.fixture.displays[0].brightness,72);
            mouseClick(find("displaySelect:screen0"));compare(find("displaySelectedPercent").text,"72%");
        }
        function test_selection_survives_reordering_and_disconnect(){
            preview.view.select("screen3",false);preview.fixture.displays=preview.fixture.displays.slice().reverse();wait(30);compare(preview.view.selected.id,"screen3");
            preview.fixture.displays=preview.fixture.displays.filter(d=>d.id!=="screen3");wait(30);compare(preview.view.selectedId,"screen4");
            preview.fixture.displays=[];wait(30);compare(preview.view.selected,null);verify(!find("displayIdentify").enabled);
        }
        function test_all_displays_preserve_dragged_slider(){
            mouseClick(find("displayExpandAll"));wait(40);verify(preview.view.allExpanded);verify(!search(preview,"displayFeaturedBrightness"));
            const slider=find("displaySlider:screen0");mousePress(slider,slider.width*0.2,slider.height/2);mouseMove(slider,slider.width*0.45,slider.height/2);wait(25);
            verify(slider===find("displaySlider:screen0"));mouseMove(slider,slider.width*0.8,slider.height/2);mouseRelease(slider,slider.width*0.8,slider.height/2);wait(25);verify(preview.fixture.displays[0].brightness>70);
            mouseClick(find("displayRowSelect:screen0"));verify(!preview.view.allExpanded);
        }
        function test_disconnect_during_drag_does_not_change_replacement(){
            const slider=find("displaySlider:screen0");mousePress(slider,slider.width*0.3,slider.height/2);wait(20);
            preview.fixture.displays=preview.fixture.displays.slice(1);wait(30);const unchanged=preview.fixture.displays[0].brightness;
            mouseMove(slider,slider.width*0.8,slider.height/2);mouseRelease(slider,slider.width*0.8,slider.height/2);wait(30);compare(preview.fixture.displays[0].brightness,unchanged);
        }
        function test_native_records_ranges_and_stable_numbers(){
            nativeModel.clear();nativeModel.append({displayName:"a",label:"Screen A",brightness:8000,maxBrightness:10000});nativeModel.append({displayName:"b",label:"Screen B",brightness:40,maxBrightness:100});wait(40);
            compare(nativeRecords.entries.length,2);const number=nativeRecords.entries[1].number;
            nativeModel.setProperty(0,"brightness",4500);wait(30);compare(nativeRecords.entries[0].brightness,4500);compare(nativeRecords.entries[0].maximum,10000);
            nativeModel.remove(0);wait(30);compare(nativeRecords.entries[0].number,number);
            nativeModel.append({displayName:"a",label:"Screen A",brightness:5000,maxBrightness:10000});wait(30);compare(nativeRecords.entries[1].number,number);
        }
        function test_theme_and_native_actions(){
            scrollBottom();mouseClick(find("displayDarkMode"));wait(30);verify(preview.fixture.darkMode);compare(find("displayCurrentTheme").text,"Sequoia Eleven Dark");compare(find("displayOtherTheme").text,"Switch to Sequoia Eleven Light");
            mouseClick(find("displayThemeSettings"));compare(preview.fixture.lastAction,"themeSettings");mouseClick(find("displaySettings"));compare(preview.fixture.lastAction,"settings");mouseClick(find("displayIdentify"));compare(preview.fixture.lastAction,"identify");
        }
        function test_narrow_large_count_and_fixed_footer(){
            for(const count of [1,2,5,7,0]){preview.fixture.reset(count);wait(30);for(const width of [292,418,552]){stage.width=width;wait(30);
                const island=find("displayActionIsland");const pos=island.mapToItem(preview,0,0);verify(pos.x>=0 && pos.x+island.width<=width);const y=pos.y;
                preview.view.allExpanded=true;wait(20);scrollBottom();compare(island.mapToItem(preview,0,0).y,y);preview.view.allExpanded=false;
                if(count>5)verify(!!find("displayOverflowSelector"));
            }}
        }
        function test_overflow_menu_selection(){
            preview.fixture.reset(7);wait(40);mouseClick(find("displayOverflowSelector"));wait(100);
            const choice=findChild(preview,"displayMenuSelect:screen6");verify(!!choice);mouseClick(choice);wait(60);compare(preview.view.selectedId,"screen6");
        }
        function test_night_light_external_inhibition(){
            scrollBottom();preview.nightFixture.inhibited=true;preview.nightFixture.togglable=false;wait(30);
            verify(find("displayNightSuspend").checked);verify(!find("displayNightSuspend").enabled);
            preview.nightFixture.togglable=true;mouseClick(find("displayNightSuspend"));wait(30);verify(!preview.nightFixture.inhibited);
            mouseClick(find("displayNightConfigure"));compare(preview.fixture.lastAction,"nightSettings");
        }
        function test_native_ranges_and_keyboard_steps(){
            preview.fixture.displays=[{id:"raw",number:1,name:"Native range",brightness:5000,maximum:10000}];wait(30);const slider=find("displaySlider:raw");compare(slider.to,10000);compare(slider.brightnessStep,100);compare(find("displaySelectedPercent").text,"50%");
            preview.fixture.keyboardAvailable=true;wait(30);const keyboard=find("displaySlider:keyboard");compare(keyboard.to,3);compare(keyboard.stepSize,1);
        }
    }
}
