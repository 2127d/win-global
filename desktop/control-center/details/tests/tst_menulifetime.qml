// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC
import QtTest
import org.kde.plasma.extras as PlasmaExtras
import plasma.applet.org.kde.plasma.notifications as Notifications
import ".." as Details

Item {
    id: stage
    width: 440; height: 500
    property QtObject sequoiaPopupScope: QtObject { property bool expanded: true }
    Window {
        id: popupWindow
        width: 440; height: 500
        visible: true
        Item {
            id: content
            anchors.fill: parent
            Details.DetailInsetMenu {
                id: insetMenu
                QQC.MenuItem { text: "First action"; onTriggered: testCase.actions++ }
            }
            Details.DetailNativeMenu {
                id: nativeMenu
                visualParent: content
                PlasmaExtras.MenuItem { text: "Native action" }
            }
            QQC.ComboBox {
                id: selector
                model: ["First", "Second"]
                property QtObject lifetime: Details.DetailPopupLifetime {
                    owner: selector
                    popup: selector.popup
                }
            }
            Notifications.FileMenu {
                id: fileMenu
                property QtObject lifetime: Details.DetailNativePopupLifetime {
                    popup: fileMenu
                    owner: fileMenu.visualParent
                }
                visualParent: content
                url: "file:///tmp"
            }

        }
    }
    TestCase {
        id: testCase
        name: "TrayMenuLifetime"
        when: windowShown
        property int actions: 0
        function init() {
            stage.sequoiaPopupScope.expanded = true;
            content.visible = true;
            popupWindow.show();
            wait(30);
        }
        function cleanup() {
            insetMenu.sequoiaLifetime.dismiss();
            nativeMenu.close();
            selector.lifetime.dismiss();
            fileMenu.lifetime.dismiss();
            wait(20);
        }
        function test_close_data() {
            const rows = [];
            for (const kind of ["menu", "native", "selector", "file"])
                for (const trigger of ["content", "window", "scope"])
                    rows.push({tag: kind + "-" + trigger, kind, trigger});
            return rows;
        }
        function isOpen(kind) {
            return kind === "native" ? nativeMenu.status === PlasmaExtras.Menu.Open
                : kind === "selector" ? selector.popup.visible
                : kind === "file" ? fileMenu.visible : insetMenu.visible;
        }
        function open(kind) {
            if (kind === "native") nativeMenu.open(0, 0);
            else if (kind === "selector") selector.popup.open();
            else if (kind === "file") fileMenu.open(0, 0);
            else insetMenu.open();
        }
        function test_close(data) {
            open(data.kind);
            tryVerify(() => isOpen(data.kind));
            if (data.trigger === "content") content.visible = false;
            else if (data.trigger === "window") popupWindow.hide();
            else stage.sequoiaPopupScope.expanded = false;
            tryVerify(() => !isOpen(data.kind));
            tryVerify(() => nativeMenuProbe.visibleMenus() === 0);
            content.visible = true;
            popupWindow.show();
            stage.sequoiaPopupScope.expanded = true;
            wait(40);
            verify(!isOpen(data.kind), "Old menu must stay closed on reopen");
            open(data.kind);
            tryVerify(() => isOpen(data.kind), 1000, "Menu must still open normally");
        }
        function test_action_still_works() {
            insetMenu.open();
            tryCompare(insetMenu, "opened", true);
            const before = actions;
            mouseClick(insetMenu.itemAt(0));
            compare(actions, before + 1);
        }

    }
}
