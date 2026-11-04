// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import ".."
Item {
    id:host
    Component { id: sample; ControlPopupGeometry {} }
    TestCase {
        name:"PopupGeometry";when:windowShown
        property var geometry
        function init() {geometry=createTemporaryObject(sample,host,{targetWidth:418,targetHeight:240,duration:240});verify(geometry);geometry.active=true;tryCompare(geometry,"ready",true);}
        function test_open_snaps_but_content_changes_animate() {
            compare(geometry.height,240);verify(!geometry.running);
            geometry.targetHeight=650;
            compare(geometry.height,240); // never resize synchronously inside layout
            wait(80);verify(geometry.height>240 && geometry.height<650);verify(geometry.running);
            tryCompare(geometry,"height",650);verify(!geometry.running);
            geometry.targetHeight=260;wait(80);verify(geometry.height>260 && geometry.height<650);
            tryCompare(geometry,"height",260);
        }
        function test_fast_filter_changes_retarget_without_jump() {
            geometry.targetHeight=700;wait(80);const previous=geometry.height;
            geometry.targetHeight=300;compare(geometry.height,previous);
            wait(30);verify(Math.abs(geometry.height-previous)<150);
            geometry.targetHeight=550;tryCompare(geometry,"height",550);verify(!geometry.running);
        }
        function test_hidden_stops_and_reopen_uses_current_content() {
            geometry.targetHeight=700;wait(70);geometry.active=false;const stopped=geometry.height;
            geometry.targetHeight=380;wait(100);compare(geometry.height,stopped);verify(!geometry.running);
            geometry.active=true;tryCompare(geometry,"ready",true);compare(geometry.height,380);verify(!geometry.running);
        }
        function test_reduced_motion_and_width() {
            geometry.duration=0;geometry.targetHeight=650;tryCompare(geometry,"height",650);verify(!geometry.running);
            geometry.duration=200;geometry.targetWidth=570;wait(60);verify(geometry.width>418 && geometry.width<570);tryCompare(geometry,"width",570);
        }
    }
}
