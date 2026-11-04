// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."
Item {
    id:host;width:700;height:1000
    Component {id:sample;Preview {}}
    TestCase {
        name:"DwellClicker";when:windowShown
        property var page
        function item(name) {const found=findChild(page,name);verify(!!found,name);return found;}
        function init() {page=createTemporaryObject(sample,host);verify(page);wait(60);}
        function test_all_modes_toggle_and_external_update() {
            page.fixture.selected=0;
            for (const mode of [10,11,12,13,14]) {
                mouseClick(item("dwellMode:"+mode));compare(page.fixture.selected,mode);verify(item("dwellMode:"+mode).checked);
                mouseClick(item("dwellMode:"+mode));compare(page.fixture.selected,0);
            }
            page.fixture.selected=12;verify(item("dwellMode:12").checked);compare(item("dwellStatus").text,"Click and drag");
        }
        function test_inactive_and_settings() {
            page.fixture.available=false;wait(20);
            for (const mode of [10,11,12,13,14]) {verify(!item("dwellMode:"+mode).enabled);mouseClick(item("dwellMode:"+mode));}
            compare(page.fixture.selectCalls,0);compare(item("dwellStatus").text,"Dwell clicking is off");
            mouseClick(item("dwellSettings"));compare(page.fixture.settingsCalls,1);
        }
        function test_widths_and_narrow_reflow() {
            const heading=item("dwellHeading");
            for(const name of ["dwellStatusCard","dwellModesCard"]) {
                compare(item(name).width,heading.width);compare(item(name).mapToItem(page,0,0).x,heading.x);
            }
            verify(item("dwellFooter").width<heading.width);
            const normal=page.view.naturalHeight;page.width=320;wait(30);verify(page.view.naturalHeight>normal);
            for(const mode of [10,11,12,13,14]) verify(item("dwellMode:"+mode).height>=44);
            page.fixture.settingsAllowed=false;wait(30);verify(!item("dwellFooter").visible);
        }
    }
}
