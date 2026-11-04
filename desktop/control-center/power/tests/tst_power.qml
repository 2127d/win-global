// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".." as Power
Item {
    id: stage; width: 442; height: 490
    Power.Preview { id: preview; anchors.fill: parent }
    TestCase {
        name: "PowerPresentation"; when: windowShown
        function find(name) { const item = findChild(preview, name); verify(!!item, name); return item; }
        function init() { stage.width=442; stage.height=490; preview.fixture.reset(); find("powerScroll").contentItem.contentY=0; wait(40); }
        function test_profiles_and_external_changes() {
            mouseClick(find("powerProfile:power-saver")); compare(preview.fixture.activeProfile,"power-saver");
            compare(find("powerModeTitle").text,"Power Save"); verify(find("powerProfile:power-saver").checked);
            preview.fixture.activeProfile="performance"; wait(20); verify(find("powerProfile:performance").checked); verify(!find("powerProfile:power-saver").checked);
        }
        function test_unsupported_and_inhibited_profiles() {
            preview.fixture.profiles=["balanced","power-saver"]; wait(20); verify(!find("powerProfile:performance").enabled);
            preview.fixture.profiles=["balanced","power-saver","performance"]; preview.fixture.inhibitionReason="high-operating-temperature"; wait(20);
            verify(!find("powerProfile:performance").enabled); verify(find("powerConstraint").text.includes("cools down"));
            preview.fixture.profiles=[]; preview.fixture.isTlpInstalled=true; wait(20); compare(find("powerModeTitle").text,"Automatic");
        }
        function test_failed_requests_keep_backend_state() {
            preview.fixture.failChanges=true; mouseClick(find("powerProfile:performance")); wait(20);
            compare(preview.fixture.activeProfile,"balanced"); verify(find("powerProfile:balanced").checked); verify(!find("powerProfile:performance").checked); verify(find("powerError").visible);
            mouseClick(find("powerKeepAwake")); wait(20); verify(!find("powerKeepAwake").checked); verify(!preview.fixture.isManuallyInhibited);
        }
        function test_sleep_toggle_and_external_state() {
            mouseClick(find("powerKeepAwake")); verify(preview.fixture.isManuallyInhibited); verify(find("powerSleepDescription").text.includes("screen locking are blocked"));
            preview.fixture.isManuallyInhibited=false; wait(20); verify(!find("powerKeepAwake").checked);
        }
        function test_footer_and_narrow_scrolling() {
            mouseClick(find("powerSettings")); compare(preview.fixture.lastAction,"settings"); mouseClick(find("powerEnergy")); compare(preview.fixture.lastAction,"energy");
            stage.width=300; stage.height=420; wait(40);
            const footer=find("powerActionIsland"); const pos=footer.mapToItem(stage,0,0); verify(pos.x>=0); verify(pos.x+footer.width<=stage.width);
            const flick=find("powerScroll").contentItem; verify(flick.contentHeight>flick.height); flick.contentY=flick.contentHeight-flick.height; wait(20); compare(footer.mapToItem(stage,0,0).y,pos.y);
            preview.fixture.settingsAllowed=false; preview.fixture.energyAllowed=false; wait(20); verify(!footer.visible);
        }
    }
}
