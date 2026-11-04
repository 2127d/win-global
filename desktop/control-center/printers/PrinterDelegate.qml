// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.printmanager as PrintManager
import org.kde.kitemmodels as KItemModels
import org.kde.config

Item {
    id: delegate
    required property PrintManager.PrinterModel printersModel
    required property PrintManager.JobModel printerJobsModel
    required property int printerState
    required property string iconName
    required property string info
    required property string location
    required property string stateMessage
    required property string printerName
    required property bool isDefault
    required property bool isClass
    required property int index
    implicitHeight: card.implicitHeight
    KItemModels.KSortFilterProxyModel {
        id: limitedJobs
        sourceModel: PrintManager.JobSortFilterModel {
            id: jobs
            filteredPrinters: delegate.printerName
            sourceModel: delegate.printerJobsModel
        }
        filterRowCallback: (row, parent) => row < 3
        onRowsInserted: (parent, first, last) => { if (first < 3) Qt.callLater(invalidateFilter); }
        onRowsRemoved: (parent, first, last) => { if (last < 3) Qt.callLater(invalidateFilter); }
    }
    QtObject {
        id: nativeController
        readonly property string name: delegate.info || delegate.printerName
        readonly property string icon: delegate.iconName
        readonly property string location: delegate.printersModel.showLocations ? delegate.location : ""
        readonly property bool isDefault: delegate.isDefault
        readonly property bool isGroup: delegate.isClass
        readonly property bool paused: delegate.printerState === 5
        readonly property bool printing: delegate.printerState === 4
        readonly property string status: delegate.stateMessage || (paused ? qsTr("Paused") : printing ? qsTr("Printing…") : qsTr("Ready"))
        readonly property bool available: delegate.printersModel.serverState !== PrintManager.ServerState.Unavailable
        readonly property bool settingsAllowed: KAuthorized.authorizeControlModule("kcm_printer_manager")
        readonly property int activeCount: jobs.activeCount
        readonly property int jobCount: jobs.count
        readonly property var jobsModel: limitedJobs
        function togglePaused() {
            if (!available) return;
            if (paused) PrintManager.PrinterCommands.resumePrinter(delegate.printerName);
            else PrintManager.PrinterCommands.pausePrinter(delegate.printerName);
        }
        function openQueue() { if (available) PrintManager.ProcessRunner.openPrintQueue(delegate.printerName); }
        function configure() { if (settingsAllowed) PrintManager.ProcessRunner.kcmConfigurePrinter(delegate.printerName); }
        function cancelJob(job) { if (available && job.jobCancelEnabled) delegate.printerJobsModel.cancel(job.jobPrinter, job.jobId); }
    }
    PrinterCard { id: card; width: parent.width; controller: nativeController }
}
