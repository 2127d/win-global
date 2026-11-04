// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."
Item {
    id: host; width: 700; height: 1000
    Component { id: previewComponent; Preview {} }
    TestCase {
        id: tests; name: "DisksAndDevices"; when: windowShown
        property var page
        function item(name) { const found=findChild(page,name);verify(!!found,name);return found; }
        function init() { page=createTemporaryObject(previewComponent,host);verify(page);wait(60); }
        function test_actions_and_busy_guards() {
            mouseClick(item("diskDefaultAction")); compare(page.drive.defaultCalls,1);
            mouseClick(item("diskAction:open")); compare(page.drive.lastAction,"open");
            verify(!item("diskAction:check").visible);
            mouseClick(item("diskMoreActions")); wait(30);
            for (const action of ["mount","check","repair"]) { mouseClick(item("diskAction:"+action));compare(page.drive.lastAction,action); }
            page.drive.busy=true; wait(30); verify(item("diskBusy").visible);
            compare(item("diskDefaultAction").enabled,false);
            for (const action of ["open","mount","check","repair"]) compare(item("diskAction:"+action).enabled,false);
        }
        function test_filters_and_footer() {
            for (const filter of ["internal","all","removable"]) { mouseClick(item("diskFilter:"+filter));compare(page.fixture.filter,filter); }
            mouseClick(item("diskSettings")); compare(page.fixture.settingsCalls,1);
            mouseClick(item("diskRemoveAll")); compare(page.fixture.removeCalls,1);
            mouseClick(item("diskAutoPopup")); compare(page.fixture.autoPopup,false);
            mouseClick(item("diskAutoPopup")); compare(page.fixture.autoPopup,true);
            page.fixture.unmountableCount=0; page.fixture.settingsAllowed=false;wait(30);
            verify(!item("diskRemoveAll").visible); verify(!item("diskSettings").visible);
        }
        function test_widths_growth_shrink_and_empty() {
            const heading=item("diskHeading");
            for (const name of ["diskFilters","diskDeviceCard"]) {
                const card=item(name); compare(Math.round(card.width),Math.round(heading.width));
                compare(Math.round(card.mapToItem(page,0,0).x),Math.round(heading.x));
            }
            verify(item("diskFooter").width<heading.width);
            const compact=page.view.naturalHeight;
            mouseClick(item("diskMoreActions"));wait(40);verify(page.view.naturalHeight>compact+50);
            mouseClick(item("diskMoreActions"));wait(40);fuzzyCompare(page.view.naturalHeight,compact,1);
            page.fixture.devicesModel=2;wait(40);verify(page.view.naturalHeight>compact+100);
            page.fixture.devicesModel=0;wait(40);verify(item("diskEmptyCard").visible);
            compare(item("diskEmptyCard").width,heading.width);verify(page.view.naturalHeight<compact);
        }
        function test_zero_free_and_long_operation() {
            page.drive.usedFraction=1;page.drive.capacityText="0 B free of 256 GB";
            verify(page.drive.lowSpace);verify(item("diskCapacityText").visible);
            compare(item("diskCapacityText").text,"0 B free of 256 GB");
            page.drive.busy=true;page.drive.status="Don’t unplug yet. Files are still being transferred…";
            page.width=330;wait(30);
            verify(item("diskStatus").height>0);compare(item("diskDeviceCard").width,page.view.width);
        }
    }
}
