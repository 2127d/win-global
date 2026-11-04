// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../details" as Details

Rectangle {
    id: preview
    width: 442; height: Math.min(760, view.naturalHeight + 76)
    color: Kirigami.Theme.backgroundColor
    property alias fixture: fixture
    property alias view: view
    QtObject {
        id: fixture
        property var layouts: [
            {iconName: "osd-shutd-laptop", label: "Only use external screen", action: 1},
            {iconName: "osd-shutd-screen", label: "Only use built-in screen", action: 2},
            {iconName: "osd-duplicate", label: "Mirror screens", action: 3},
            {iconName: "osd-sbs-left", label: "Extend to left of built-in screen", action: 4},
            {iconName: "osd-sbs-sright", label: "Extend to right of built-in screen", action: 5}
        ]
        property int connectedCount: 2
        property bool presentationEnabled: false
        property bool presentationBusy: false
        property bool powerAvailable: true
        property string errorMessage: ""
        property var inhibitions: []
        property bool settingsAllowed: true
        property int lastAction: -1
        property int settingsCalls: 0
        property int presentationCalls: 0
        function applyLayout(action) { lastAction = action; }
        function setPresentationMode(enabled) { presentationEnabled = enabled; presentationCalls++; }
        function settings() { settingsCalls++; }
        function reset() { connectedCount=2;presentationEnabled=false;presentationBusy=false;powerAvailable=true;errorMessage="";inhibitions=[];settingsAllowed=true;lastAction=-1;settingsCalls=0;presentationCalls=0; }
    }
    Details.DetailSurface {
        id: heading
        x: 12; y: 12; width: parent.width - 24; height: 38; radius: 19; elevated: true
        Kirigami.Heading { anchors.centerIn: parent; text: "Display Configuration"; level: 3 }
    }
    DisplayConfigurationView {
        id: view
        x: 12; y: 60; width: parent.width - 24; height: parent.height - y - 12
        controller: fixture
    }
}
