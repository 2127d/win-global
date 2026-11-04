// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Configuration
Item {
    width: 442; height: 760
    Configuration.Preview { id: preview }
    TestCase {
        name: "DisplayConfiguration"; when: windowShown
        function find(name) { const item=findChild(preview,name);verify(!!item,name);return item; }
        function init() { preview.width=442;preview.fixture.reset();wait(40); }
        function test_all_native_action_values_forwarded() {
            for(let i=0;i<preview.fixture.layouts.length;i++) {
                mouseClick(find("configurationPreset:"+preview.fixture.layouts[i].action));compare(preview.fixture.lastAction,preview.fixture.layouts[i].action);
            }
        }
        function test_one_display_disables_layouts_but_keeps_settings() {
            preview.fixture.connectedCount=1;wait(30);
            for(let i=0;i<preview.fixture.layouts.length;i++) verify(!find("configurationPreset:"+preview.fixture.layouts[i].action).enabled);
            mouseClick(find("configurationPreset:1"));compare(preview.fixture.lastAction,-1);
            mouseClick(find("configurationSettings"));compare(preview.fixture.settingsCalls,1);
        }
        function test_presentation_keyboard_and_pending_job() {
            const toggle=find("configurationPresentationSwitch");toggle.forceActiveFocus();keyClick(Qt.Key_Space);
            compare(preview.fixture.presentationEnabled,true);compare(preview.fixture.presentationCalls,1);
            preview.fixture.presentationBusy=true;wait(20);verify(!toggle.enabled);
            mouseClick(toggle);compare(preview.fixture.presentationCalls,1);
            preview.fixture.presentationBusy=false;preview.fixture.powerAvailable=false;wait(20);verify(!toggle.enabled);
        }
        function test_content_height_and_shared_widths() {
            const height=preview.view.naturalHeight;
            preview.fixture.inhibitions=[{Name:"Presentation application",Icon:"applications-office",Reason:"Showing slides"},{Name:"Video player",Reason:"Playing a video"}];wait(30);
            verify(preview.view.naturalHeight>height);
            for(const name of ["configurationLayoutCard","configurationPresentationCard"]) compare(find(name).width,preview.view.width);
            verify(find("configurationFooter").width<preview.view.width);
            preview.fixture.inhibitions=[];wait(30);compare(preview.view.naturalHeight,height);
        }
        function test_small_width_error_and_restricted_settings() {
            preview.width=320;preview.fixture.errorMessage="Power management could not complete this request.";preview.fixture.settingsAllowed=false;wait(30);
            compare(find("configurationPresetGrid").columns,1);
            verify(find("configurationError").visible);verify(!find("configurationFooter").visible);
        }
    }
}
