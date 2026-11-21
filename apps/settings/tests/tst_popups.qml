// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as C
import QtTest
import org.kde.kirigami as K
import org.sequoia.components as S

Item {
    width: 640; height: 360
    C.Button {
        id: button; x: 250; y: 230; text: "Global Theme"
        C.ToolTip.text: "Global Theme"
        C.ToolTip.delay: 0
    }
    C.Menu {
        id: menu
        K.Theme.inherit: false
        K.Theme.highlightColor: "#dd3344"
        C.MenuItem { id: row; text: "Configure…"; K.Theme.inherit: false }
        C.MenuItem { text: "Disabled action"; enabled: false }
    }
    S.Theme { id: theme; K.Theme.inherit: false; K.Theme.highlightColor: "#dd3344" }
    S.MenuHighlight { id: custom; theme: theme; width: 180; height: 36; highlighted: true }
    TestCase {
        name: "HubPopupStyle"; when: windowShown
        function test_tooltip_data() {
            return [{tag:"short",label:"Icons"}, {tag:"long",label:"Configure application appearance, window decorations, fonts and desktop effects"}];
        }
        function test_tooltip(data) {
            button.C.ToolTip.text = data.label;
            button.C.ToolTip.show(data.label, 0);
            const tip = button.C.ToolTip.toolTip;
            tryCompare(tip, "visible", true);
            compare(tip.background.objectName, "sequoiaTooltipCapsule");
            compare(tip.background.radius, Math.min(tip.background.width, tip.background.height) / 2);
            compare(tip.leftInset, 0); compare(tip.topInset, 0);
            verify(tip.background.antialiasing);
            wait(200);
            let image = grabImage(tip.background);
            verify(image.width > 0 && image.height > 0);
            button.C.ToolTip.hide();
        }
        function test_accentGlass() {
            menu.open(); tryCompare(menu, "visible", true);
            row.highlighted = true;
            tryCompare(row.background, "opacity", 1);
            for (const accent of ["#dd3344", "#3366dd"]) {
                row.K.Theme.highlightColor = accent;
                theme.K.Theme.highlightColor = accent;
                tryCompare(row.background, "accentColor", accent);
                tryCompare(custom, "accentColor", accent);
                verify(row.background.color.a > 0 && row.background.color.a < .5);
                verify(custom.color.a > 0 && custom.color.a < .5);
                verify(row.background.children[0].gradient !== undefined);
                verify(custom.children[0].gradient !== undefined);
            }
            row.enabled = false;
            tryCompare(row.background, "opacity", 0);
            menu.close();
        }
    }
}
