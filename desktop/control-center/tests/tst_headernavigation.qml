// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC
import QtTest
import ".." as Tray

Item {
    width: 320
    height: 80
    Tray.ControlHeaderRow {
        id: header
        QQC.ToolButton { id: back; text: "Back" }
        Loader { id: extra; sourceComponent: QQC.Switch {} }
        QQC.ToolButton { id: options; text: "More" }
        QQC.ToolButton { id: pin; text: "Pin" }
    }
    TestCase {
        name: "HeaderNavigation"
        when: windowShown
        function test_arrows_follow_visible_controls() {
            back.forceActiveFocus(Qt.TabFocusReason);
            keyClick(Qt.Key_Right);
            verify(extra.item.activeFocus);
            keyClick(Qt.Key_Right);
            verify(options.activeFocus);
            extra.active = false;
            keyClick(Qt.Key_Left);
            verify(back.activeFocus);
            options.enabled = false;
            keyClick(Qt.Key_Right);
            verify(pin.activeFocus);
            header.LayoutMirroring.enabled = true;
            waitForItemPolished(header);
            keyClick(Qt.Key_Right);
            verify(back.activeFocus);
        }
    }
}
