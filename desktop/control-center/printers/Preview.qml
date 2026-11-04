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
    property alias printer: printer
    property alias view: view
    QtObject {
        id: fixture
        property int printersModel: 1
        property bool serverUnavailable: false
        property string error: ""
        property bool settingsAllowed: true
        property string query: ""
        property int settingsCalls: 0
        property int refreshCalls: 0
        function search(text) { query=text; }
        function settings() { settingsCalls++; }
        function refresh() { refreshCalls++; }
    }
    QtObject {
        id: printer
        property string name: "Office printer"
        property string location: "Study"
        property string icon: "printer"
        property bool isDefault: true
        property bool isGroup: false
        property bool paused: false
        property bool printing: false
        property string status: paused ? "Paused" : "Ready"
        property bool available: true
        property bool settingsAllowed: true
        property int activeCount: 2
        property int jobCount: 2
        property var jobsModel: [
            {jobId:"21",jobPrinter:"Office",jobName:"Project notes.pdf",jobIconName:"application-pdf",jobCancelEnabled:true},
            {jobId:"22",jobPrinter:"Office",jobName:"Reference images.pdf",jobIconName:"application-pdf",jobCancelEnabled:false}
        ]
        property int pauseCalls: 0
        property int queueCalls: 0
        property int configureCalls: 0
        property string lastCancelled: ""
        function togglePaused() { paused=!paused;pauseCalls++; }
        function openQueue() { queueCalls++; }
        function configure() { configureCalls++; }
        function cancelJob(job) { lastCancelled=job.jobId; }
    }
    Details.DetailSurface {
        objectName: "printerHeading"
        x:12;y:12;width:parent.width-24;height:38;radius:19;elevated:true
        Kirigami.Heading { anchors.centerIn: parent; text:"Printers";level:3 }
    }
    PrintersView {
        id:view;x:12;y:60;width:parent.width-24;height:parent.height-y-12
        controller:fixture
        printerDelegate: Component { PrinterCard { Layout.fillWidth:true;controller:printer } }
    }
}
