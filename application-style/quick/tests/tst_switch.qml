// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.ksvg as KSvg
import QtTest
import QtQuick.Controls as QQC2
import org.kde.desktop as Desktop
import org.kde.plasma.components as Plasma
import org.kde.kirigami as Kirigami

Item {
    Component.onCompleted: {
        KSvg.ImageSet.basePath = "plasma/desktoptheme";
        KSvg.ImageSet.imageSetName = Kirigami.Theme.backgroundColor.hslLightness < 0.5 ? "sequoia-eleven-dark" : "sequoia-eleven-light";
    }

    width: 480; height: 300
    Desktop.Switch { id: desktop; x: 30; y: 40; text: "Desktop switch" }
    Desktop.SwitchDelegate { id: delegate; x: 30; y: 90; text: "Desktop delegate" }
    Plasma.Switch { id: plasma; x: 270; y: 40; text: "Plasma switch" }
    Desktop.MenuItem { id: menuToggle; x: 30; y: 160; width: 250; text: "Menu toggle"; checkable: true }
    QQC2.ActionGroup { id: choices; exclusive: true }
    QQC2.Action { id: choiceA; text: "Choice A"; checkable: true; checked: true; QQC2.ActionGroup.group: choices }
    QQC2.Action { id: choiceB; text: "Choice B"; checkable: true; QQC2.ActionGroup.group: choices }
    Desktop.MenuItem { id: menuRadio; x: 30; y: 210; width: 250; action: choiceB }
    TestCase {
        name: "AccentSwitches"
        when: windowShown
        function init() {
            for (const control of [desktop, delegate, plasma, menuToggle]) {
                control.checked = false;
                control.enabled = true;
                control.LayoutMirroring.enabled = false;
            }
            wait(300);
        }
        function test_nativeInteractions_data() {
            return [{tag: "desktop", control: desktop}, {tag: "delegate", control: delegate}, {tag: "plasma", control: plasma}];
        }
        function test_nativeInteractions(data) {
            const control = data.control;
            mouseClick(control);
            compare(control.checked, true);
            control.forceActiveFocus(Qt.TabFocusReason);
            keyClick(Qt.Key_Space);
            compare(control.checked, false);
            control.enabled = false;
            mouseClick(control);
            compare(control.checked, false);
        }
        function test_accentTracks() {
            desktop.checked = true;
            const track = findChild(desktop, "sequoiaSwitchTrack");
            verify(track !== null);
            for (const accent of ["#d946b2", "#21956b", "#2463d4"]) {
                desktop.Kirigami.Theme.highlightColor = accent;
                tryCompare(track, "color", accent);
            }
        }
        function test_thumbGeometryAndRtl() {
            const indicator = desktop.indicator;
            const thumb = findChild(desktop, "sequoiaSwitchThumb");
            verify(thumb.width < indicator.height);
            verify(indicator.width > indicator.height);
            compare(thumb.color, "#ffffff");
            const left = thumb.x;
            desktop.checked = true;
            wait(300);
            verify(thumb.x > left);
            verify(thumb.x + thumb.width < indicator.width);
            desktop.LayoutMirroring.enabled = true;
            wait(300);
            fuzzyCompare(thumb.x, left, 0.5);
            desktop.checked = false;
            wait(300);
            verify(thumb.x > left);
        }
        function test_drag() {
            const thumb = findChild(desktop, "sequoiaSwitchThumb");
            const point = thumb.mapToItem(desktop, thumb.width / 2, thumb.height / 2);
            mouseDrag(desktop, point.x, point.y, desktop.indicator.width, 0);
            compare(desktop.checked, true);
        }
        function test_menuToggle() {
            const track = findChild(menuToggle, "sequoiaSwitchTrack");
            verify(track !== null);
            mouseClick(menuToggle);
            compare(menuToggle.checked, true);
            tryCompare(track, "color", menuToggle.Kirigami.Theme.highlightColor);
            menuToggle.forceActiveFocus(Qt.TabFocusReason);
            keyClick(Qt.Key_Space);
            compare(menuToggle.checked, false);
            menuToggle.LayoutMirroring.enabled = true;
            wait(300);
            const thumb = findChild(menuToggle, "sequoiaSwitchThumb");
            verify(thumb.x > 2);
            menuToggle.enabled = false;
            mouseClick(menuToggle);
            compare(menuToggle.checked, false);
        }
        function test_menuExclusiveChoice() {
            compare(choiceB.QQC2.ActionGroup.group, choices);
            compare(menuRadio.action, choiceB);
            compare(menuRadio.indicator.exclusive, true);
            verify(!findChild(menuRadio, "sequoiaSwitchTrack"));
            mouseClick(menuRadio);
            compare(choiceB.checked, true);
            compare(choiceA.checked, false);
        }
        function test_keyboardFocusRing() {
            delegate.forceActiveFocus(Qt.MouseFocusReason);
            desktop.forceActiveFocus(Qt.TabFocusReason);
            tryCompare(findChild(desktop, "sequoiaSwitchFocus"), "visible", true);
        }
    }
}
