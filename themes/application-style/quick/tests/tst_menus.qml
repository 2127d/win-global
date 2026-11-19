import QtQuick
import QtQuick.Controls as C
import QtTest
import org.sequoia.components 1.0 as S

Item {
    id: host
    width: 720; height: 640
    C.Button { id: menuTrigger; x: 40; y: 20; text: "Menu" }
    S.Theme { id: testTheme; mode: "system"; shaderEnabled: true; reducedMotion: true }
    C.Menu {
        id: nativeMenu
        popupType: C.Popup.Item
        x: 40; y: 65
        C.ActionGroup { id: nativeGroup }
        C.MenuItem { text: "Show previews"; checkable: true; icon.name: "document-preview" }
        C.MenuItem { action: C.Action { text: "Name"; checkable: true; checked: true; C.ActionGroup.group: nativeGroup } }
        C.MenuItem { action: C.Action { text: "Date"; checkable: true; C.ActionGroup.group: nativeGroup } }
        C.MenuItem { text: "Unavailable"; enabled: false }
        C.MenuItem { text: "Hidden"; visible: false }
        C.MenuSeparator {}
        C.Menu { title: "More"; C.MenuItem { text: "Details" } }
    }
    S.PopupMenu {
        id: sharedMenu
        theme: testTheme; trigger: menuTrigger; backdropItem: host
        C.ActionGroup { id: sharedGroup }
        S.MenuItem { text: "Show previews"; checkable: true; icon.name: "document-preview" }
        S.MenuItem { action: C.Action { text: "Name"; checkable: true; checked: true; C.ActionGroup.group: sharedGroup } }
        S.MenuItem { action: C.Action { text: "Date"; checkable: true; C.ActionGroup.group: sharedGroup } }
        S.MenuItem { text: "Unavailable"; enabled: false }
        S.MenuItem { text: "Hidden"; visible: false }
        C.MenuSeparator {}
        C.Menu { title: "More"; C.MenuItem { text: "Details" } }
    }
    TestCase {
        name: "SharedQuickMenus"
        when: windowShown
        function cleanup() { nativeMenu.close(); sharedMenu.close(); wait(250); }
        function test_menus_data() {
            return [{tag:"desktop", menu:nativeMenu, rtl:false}, {tag:"custom", menu:sharedMenu, rtl:false},
                    {tag:"desktop-rtl", menu:nativeMenu, rtl:true}, {tag:"custom-rtl", menu:sharedMenu, rtl:true}];
        }
        function test_menus(data) {
            const menu=data.menu;
            menu.locale=Qt.locale(data.rtl ? "ar" : "en");
            menu.itemAt(0).checked=false; menu.itemAt(1).checked=true;
            menu.open(); tryCompare(menu, "opened", true);
            compare(menu.padding || menu.topPadding, 8);
            const surface=menu===sharedMenu ? findChild(menu.background,"sequoiaMenuSurface") : menu.background;
            verify(surface); compare(surface.radius,24);
            compare(surface.color, nativeMenu.background.color);
            const toggle=menu.itemAt(0), first=menu.itemAt(1), second=menu.itemAt(2);
            verify(toggle.indicator.width===42 && first.indicator.width===42, "Aligned toggle/radio column");
            compare(toggle.background.radius,13);
            toggle.highlighted=true; compare(toggle.background.color.a.toFixed(2),"0.14");
            verify(findChild(toggle,"sequoiaSwitchTrack")!==null, "Native switch delegate");
            compare(menu.itemAt(4).height,0);
            const old=toggle.checked;
            waitForRendering(toggle);
            wait(250); // Settle palette transitions before comparing the painted frame.
            compare(findChild(toggle,"sequoiaSwitchTrack").color, toggle.indicator.item.offColor);
            grabImage(host).save("menus-"+data.tag+".png");
            mouseClick(toggle); compare(toggle.checked,!old); tryCompare(menu,"visible",false);
            menu.open(); tryCompare(menu,"opened",true);
            mouseClick(second); verify(second.checked && !first.checked); tryCompare(menu,"visible",false);
            menu.open(); tryCompare(menu,"opened",true);
            mouseClick(menu.itemAt(3)); verify(menu.visible, "Disabled action stays inert");
            const more=menu.itemAt(6); verify(more.subMenu);
            menu.currentIndex=6; more.forceActiveFocus(); keyClick(menu.mirrored ? Qt.Key_Left : Qt.Key_Right);
            tryCompare(more.subMenu,"visible",true);
            const parentPoint=menu.background.mapToItem(host,0,0);
            const childPoint=more.subMenu.background.mapToItem(host,0,0);
            const gap=Math.max(childPoint.x-parentPoint.x-menu.width, parentPoint.x-childPoint.x-more.subMenu.width);
            verify(gap >= 5.5, "Cascading Quick menus keep a visible gap: " + gap);
            keyClick(Qt.Key_Escape); tryCompare(more.subMenu,"visible",false); verify(menu.visible);
            keyClick(Qt.Key_Escape); tryCompare(menu,"visible",false);
        }
    }
}
