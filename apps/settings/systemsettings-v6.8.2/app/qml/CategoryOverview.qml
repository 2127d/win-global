// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
ColumnLayout {
    id: page
    property string group: navigation.selectedGroup
    spacing: 18
    function entry(id) { return navigation.entries.find(e => e.id === id); }
    function section(title, ids) { return {title:title, entries:ids.map(id=>entry(id)).filter(e=>e !== undefined)}; }
    readonly property var structure: ({
        system:[section(i18n("Users & security"),["kcm_users","kcm_kwallet5","kcm_screenlocker","kcm_plasmalogin"]),section(i18n("Language & time"),["kcm_regionandlang","kcm_clock","kcm_spellchecking"]),section(i18n("Maintenance"),["kcm_updates","kcm_kded","kcm_qtquicksettings"]),section(i18n("Personal preferences"),["kcm_desktoppaths","kcm_recentFiles","kcm_feedback","kcm_landingpage"])],
        display:[section(i18n("Screens & audio"),["kcm_kscreen","kcm_pulseaudio"]),section(i18n("Comfortable viewing"),["kcm_nightlight","kcm_nighttime"])],
        devices:[section(i18n("Everyday devices"),["kcm_keyboard","kcm_mouse","kcm_touchpad","kcm_bluetooth"]),section(i18n("Input & controllers"),["kcm_tablet","kcm_gamecontroller","kcm_virtualkeyboard","kcm_touchscreen","kcm_virtualkeyboard_wayland"])],
        network:[section(i18n("Connections & sharing"),["kcm_networkmanagement","kcm_netpref","kcm_proxy","kcm_krdpserver"]),section(i18n("Online services"),["kcm_kaccounts","kcm_push_notifications","kcm_webshortcuts"])],
        desktop:[section(i18n("Everyday controls"),["kcm_componentchooser","kcm_notifications","kcm_keys","kcm_kwin_effects"]),section(i18n("Workspaces & multitasking"),["kcm_activities","kcm_kwin_virtualdesktops","kcm_kwintabbox","kcm_kwinscreenedges","kcm_kwintouchscreen"]),section(i18n("Search & files"),["kcm_plasmasearch","kcm_baloofile","kcm_filetypes","kcm_cddb"]),section(i18n("App startup & permissions"),["kcm_autostart","kcm_app-permissions","kcm_smserver","kcm_kwinxwayland"]),section(i18n("Window behavior & rules"),["kcm_kwinoptions","kcm_kwinrules","kcm_kwin_scripts","kcm_workspace"])],
        power:[],accessibility:[]
    })
    readonly property var sections: {
        let listed = structure[group] || [];
        let used = []; listed.forEach(s=>s.entries.forEach(e=>used.push(e.id)));
        if (group === "system") used.push("kcm_about-distro");
        let other = navigation.entries.filter(e=>used.indexOf(e.id)<0);
        return other.length ? listed.concat([{title:group === "power" || group === "accessibility" ? i18n("Full controls") : i18n("More settings"),entries:other}]) : listed;
    }
    function glyph(id) {
        const map={kcm_users:"system", "kcm_about-distro":"info",kcm_kwallet5:"wallet",kcm_screenlocker:"lock",kcm_plasmalogin:"login",kcm_regionandlang:"language",kcm_clock:"clock",kcm_spellchecking:"check",kcm_updates:"refresh",kcm_kded:"settings",kcm_qtquicksettings:"chip",kcm_kscreen:"display",kcm_pulseaudio:"sound",kcm_nightlight:"eye",kcm_nighttime:"clock",kcm_keyboard:"keyboard",kcm_mouse:"mouse",kcm_touchpad:"pointer",kcm_bluetooth:"bluetooth",kcm_networkmanagement:"network",kcm_krdpserver:"display",kcm_kaccounts:"system",kcm_notifications:"bell",kcm_componentchooser:"desktop",kcm_keys:"keyboard",kcm_kwin_effects:"sparkles",kcm_powerdevilprofilesconfig:"settings",kcm_access:"accessibility",kcm_desktoppaths:"folder",kcm_recentFiles:"clock",kcm_feedback:"info",kcm_landingpage:"sliders"};
        return map[id] || "sliders";
    }
    BatteryHero { visible: page.group === "power"; Layout.fillWidth: true; Layout.preferredHeight: 176 }
    HubPanel {
        visible: page.group === "accessibility"
        Layout.fillWidth: true; implicitHeight: 112
        RowLayout { anchors.fill: parent; anchors.margins: 22; spacing: 18
            ColumnLayout { Layout.fillWidth: true
                C.Label { text: i18n("Larger text preview"); opacity: .75 }
                C.Label { Layout.fillWidth: true; text: i18n("Clearer, higher contrast text"); font.pointSize: K.Theme.defaultFont.pointSize*1.45; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
            }
            C.Label { text:"Aa"; font.pointSize: K.Theme.defaultFont.pointSize*2; font.weight: Font.DemiBold }
            HubIcon { symbol:"eye"; width:32; height:32 }
        }
    }
    GridLayout {
        visible: page.group === "power" || page.group === "accessibility"
        Layout.fillWidth: true
        columns: width < 480 ? 2 : page.group === "power" ? 3 : 4; columnSpacing: 12; rowSpacing: 12
        Repeater {
            model: page.group === "power" ? [{label:i18n("Energy saving"),icon:"leaf"},{label:i18n("Sleep & lid"),icon:"sleep"},{label:i18n("Power profiles"),icon:"gauge"}] : [{label:i18n("Vision"),icon:"eye"},{label:i18n("Hearing"),icon:"ear"},{label:i18n("Keyboard"),icon:"keyboard"},{label:i18n("Pointer"),icon:"pointer"}]
            HubTile {
                required property var modelData
                Layout.fillWidth: true; Layout.minimumWidth: 0
                label: modelData.label; symbol: modelData.icon
                Accessible.description: i18n("Open the full native controls for this section")
                onClicked: navigation.open(page.group === "power" ? "kcm_powerdevilprofilesconfig" : "kcm_access")
            }
        }
    }
    C.Label { visible: page.group === "power" || page.group === "accessibility"; Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pointSize: K.Theme.smallFont.pointSize; opacity: .68; text: i18n("These topics open the complete native settings page below.") }
    RowLayout {
        visible: page.group === "system"; Layout.fillWidth: true; spacing: 12
        HubTile { Layout.fillWidth: true; Layout.minimumWidth:0; horizontal:true; label:i18n("Your account"); detail:i18n("Manage users and account details"); symbol:"system"; accented:true; onClicked:navigation.open("kcm_users") }
        HubTile { Layout.fillWidth: true; Layout.minimumWidth:0; horizontal:true; label:i18n("About this System"); detail:i18n("Hardware and software information"); symbol:"info"; onClicked:navigation.open("kcm_about-distro") }
    }
    Repeater {
        model: page.sections.filter(s=>s.entries.length > 0)
        HubPanel {
            id: sectionPanel
            required property var modelData
            Layout.fillWidth:true
            implicitHeight: sectionBody.implicitHeight + 28
            tint: .23
            ColumnLayout {
                id: sectionBody
                anchors.fill:parent; anchors.margins:14; spacing:12
                C.Label { text: sectionPanel.modelData.title; font.weight:Font.DemiBold; Layout.fillWidth:true }
                GridLayout {
                    Layout.fillWidth:true
                    columns: width>740 && sectionPanel.modelData.entries.length===4 ? 4 : width>650 ? 3 : width>430 ? 2 : 1; columnSpacing:10; rowSpacing:10
                    Repeater {
                        model: sectionPanel.modelData.entries
                        HubTile {
                            required property var modelData
                            Layout.fillWidth:true; Layout.minimumWidth:0
                            horizontal:true; label:modelData.name; symbol:page.glyph(modelData.id)
                            accented: page.group === "power" || page.group === "accessibility"
                            onClicked:navigation.open(modelData.id)
                        }
                    }
                }
            }
        }
    }
    C.Button { Layout.fillWidth:true; text:i18n("All %1 settings",navigation.catalog.length); flat:true; onClicked:navigation.browse("all") }
}
