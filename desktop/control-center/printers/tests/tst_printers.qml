// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."
Item {
    id: host;width:700;height:1000
    Component { id: sample;Preview {} }
    TestCase {
        name:"Printers";when:windowShown
        property var page
        function item(name) { const found=findChild(page,name);verify(!!found,name);return found; }
        function init() { page=createTemporaryObject(sample,host);verify(page);wait(60); }
        function test_actions() {
            mouseClick(item("printerPause"));compare(page.printer.paused,true);compare(item("printerPause").text,"Resume");
            mouseClick(item("printerPause"));compare(page.printer.paused,false);
            mouseClick(item("printerQueue"));compare(page.printer.queueCalls,1);
            mouseClick(item("printerConfigure"));compare(page.printer.configureCalls,1);
            mouseClick(item("printerCancelJob:21"));compare(page.printer.lastCancelled,"21");
            verify(!item("printerCancelJob:22").visible);
            page.printer.available=false;wait(20);verify(!item("printerPause").enabled);verify(!item("printerCancelJob:21").enabled);
        }
        function test_search_empty_and_service_failure() {
            page.fixture.printersModel=0;wait(20);compare(item("printerEmptyTitle").text,"No printers found");
            item("printerSearch").text="Laser";compare(page.fixture.query,"Laser");compare(item("printerEmptyTitle").text,"No matching printers");
            page.fixture.serverUnavailable=true;page.fixture.error="Service offline";wait(20);
            compare(item("printerEmptyTitle").text,"Printing service unavailable");
            mouseClick(item("printersRefresh"));compare(page.fixture.refreshCalls,1);
            mouseClick(item("printersSettings"));compare(page.fixture.settingsCalls,1);
        }
        function test_aligned_widths_and_content_size() {
            const heading=item("printerHeading");
            for (const name of ["printerSearch","printerCard"]) {
                const card=item(name);compare(card.width,heading.width);compare(card.mapToItem(page,0,0).x,heading.x);
            }
            verify(item("printerFooter").width<heading.width);
            const expanded=page.view.naturalHeight;
            mouseClick(item("printerJobsToggle"));wait(30);verify(page.view.naturalHeight<expanded);
            mouseClick(item("printerJobsToggle"));wait(30);fuzzyCompare(page.view.naturalHeight,expanded,1);
            page.printer.jobCount=7;wait(30);verify(item("printerMoreJobs").visible);
            mouseClick(item("printerMoreJobs"));compare(page.printer.queueCalls,1);
            page.fixture.printersModel=2;wait(30);verify(page.view.naturalHeight>expanded+100);
        }
    }
}
