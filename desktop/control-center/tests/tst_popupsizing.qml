// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtTest
import ".." as Tray
Item {
    Item { id: small; implicitWidth: 360; implicitHeight: 210; width: 900; height: 900 }
    Item { id: tall; implicitWidth: 480; implicitHeight: 900 }
    Item { id: dynamic; property real sequoiaPreferredHeight: 250; implicitHeight: 600 }
    Item { id: native; Layout.preferredWidth: 390; Layout.preferredHeight: 310 }
    Tray.ControlPopupSizing { id: sizing; availableWidth: 1000; availableHeight: 900; headingHeight: 40 }
    TestCase {
        name: "TrayContentSizing"
        function init() { sizing.page = null; sizing.availableWidth = 1000; sizing.availableHeight = 900; dynamic.sequoiaPreferredHeight = 250; }
        function test_previous_geometry_is_not_an_input() {
            sizing.page = small; const width = sizing.preferredWidth; const height = sizing.preferredHeight;
            sizing.page = tall; verify(sizing.preferredHeight > height);
            small.width = 680; small.height = 720;
            sizing.page = small; compare(sizing.preferredWidth, width); compare(sizing.preferredHeight, height);
        }
        function test_expanded_content_grows_and_collapsed_content_shrinks() {
            sizing.page = dynamic; const compact = sizing.preferredHeight;
            dynamic.sequoiaPreferredHeight = 450; verify(sizing.preferredHeight > compact);
            dynamic.sequoiaPreferredHeight = 2000; compare(sizing.preferredHeight, 720);
            dynamic.sequoiaPreferredHeight = 250; compare(sizing.preferredHeight, compact);
        }
        function test_small_screen_and_native_hints() {
            sizing.page = native; verify(sizing.preferredWidth >= 390); verify(sizing.preferredHeight >= 310);
            sizing.availableWidth = 300; sizing.availableHeight = 260;
            verify(sizing.preferredWidth <= 300); verify(sizing.preferredHeight <= 260);
            sizing.availableHeight = 80; verify(sizing.preferredHeight <= 80);
        }
    }
}
