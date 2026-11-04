// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../details" as Details
Rectangle {
    id: preview
    width: 442; height: Math.min(790,view.naturalHeight+76)
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property alias vault: vault
    property alias view: view
    QtObject {
        id: fixture
        property int vaultsModel: 1
        property int createCalls: 0
        function create() { createCalls++; }
    }
    QtObject {
        id: vault
        property string name: "Personal files"
        property string icon: "plasmavault"
        property bool opened: false
        property bool offlineOnly: false
        property string message: ""
        property bool available: true
        property bool busy: false
        readonly property bool canAct: available && !busy
        readonly property string emblem: message ? "emblem-error" : opened ? "emblem-encrypted-unlocked" : offlineOnly ? "emblem-unmounted" : ""
        property int primaryCalls: 0
        property int secondaryCalls: 0
        property int configureCalls: 0
        property int forceCalls: 0
        function primary() { primaryCalls++; }
        function secondary() { secondaryCalls++; }
        function configure() { configureCalls++; }
        function forceLock() { forceCalls++; }
    }
    Details.DetailSurface {
        objectName: "vaultHeading"
        x:12;y:12;width:parent.width-24;height:38;radius:19;elevated:true
        Kirigami.Heading { anchors.centerIn: parent; text:"Vaults";level:3 }
    }
    VaultsView {
        id: view; x:12;y:60;width:parent.width-24;height:parent.height-y-12
        controller:fixture
        vaultDelegate: Component { VaultCard { Layout.fillWidth:true;controller:vault } }
    }
}
