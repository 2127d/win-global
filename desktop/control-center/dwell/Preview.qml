// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import "../details" as Details
Rectangle {
    id: preview
    width: 442; height: Math.min(790,view.naturalHeight+76)
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property alias view: view
    QtObject {
        id: fixture
        property bool available: true
        property int selected: 10
        property bool settingsAllowed: true
        property int settingsCalls: 0
        property int selectCalls: 0
        function select(button) { selected=selected===button?0:button;selectCalls++; }
        function settings() { settingsCalls++; }
    }
    Details.DetailSurface {
        objectName: "dwellHeading"
        x:12;y:12;width:parent.width-24;height:38;radius:19;elevated:true
        Kirigami.Heading { anchors.centerIn: parent; text:"Dwell Clicker";level:3 }
    }
    DwellView { id:view;x:12;y:60;width:parent.width-24;height:parent.height-y-12;controller:fixture }
}
