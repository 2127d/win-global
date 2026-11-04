// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Rectangle {
    id: preview
    width: 442; height: 490
    color: Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    property alias fixture: fixture
    property alias view: view
    QtObject {
        id: fixture
        property string activeProfile: "balanced"
        property var profiles: ["power-saver", "balanced", "performance"]
        property bool profilesInstalled: true
        property bool isTlpInstalled: false
        property string inhibitionReason: ""
        property string degradationReason: ""
        property var profileHolds: []
        property bool isManuallyInhibited: false
        property var requestedInhibitions: []
        property bool inhibitsLidAction: false
        property string errorMessage: ""
        property bool settingsAllowed: true
        property bool energyAllowed: true
        property bool failChanges: false
        property string lastAction: ""
        function activateProfile(profile) {
            lastAction = "profile:" + profile;
            if (failChanges) errorMessage = "Could not change the power mode. Try again.";
            else activeProfile = profile;
        }
        function setInhibited(value) {
            lastAction = "inhibit:" + value;
            if (failChanges) errorMessage = "Could not change sleep and screen locking. Try again.";
            else isManuallyInhibited = value;
        }
        function settings() { lastAction = "settings"; }
        function energy() { lastAction = "energy"; }
        function reset() {
            activeProfile = "balanced"; profiles = ["power-saver", "balanced", "performance"];
            profilesInstalled = true; isTlpInstalled = false; inhibitionReason = ""; degradationReason = "";
            profileHolds = []; isManuallyInhibited = false; requestedInhibitions = []; inhibitsLidAction = false;
            errorMessage = ""; failChanges = false; settingsAllowed = true; energyAllowed = true; lastAction = "";
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 8
        Details.DetailSurface {
            Layout.fillWidth: true; implicitHeight: 38; radius: 19; elevated: true
            RowLayout {
                anchors.fill: parent; anchors.margins: 4
                PowerActionButton { flat: true; display: PC.AbstractButton.IconOnly; icon.name: "go-previous-symbolic"; text: "Back" }
                PC.Label { Layout.fillWidth: true; text: "Power Management"; font.weight: Font.DemiBold }
                PowerActionButton { flat: true; display: PC.AbstractButton.IconOnly; icon.name: "window-pin-symbolic"; text: "Pin" }
            }
        }
        PowerView { id: view; Layout.fillWidth: true; Layout.fillHeight: true; controller: fixture }
    }
}
