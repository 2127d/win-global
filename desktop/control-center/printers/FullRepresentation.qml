// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.printmanager as PrintManager
import org.kde.kitemmodels as KItemModels
import org.kde.config
import org.kde.kcmutils

PlasmaExtras.Representation {
    id: full
    objectName: "sequoiaDetailPage"
    required property PrintManager.JobModel printerJobsModel
    required property PrintManager.PrinterModel printersModel
    required property bool serverUnavailable
    required property string printersModelError
    property string actionError: ""
    readonly property real sequoiaPreferredWidth: 418
    readonly property real sequoiaPreferredHeight: view.naturalHeight
    padding: 0; background: null
    KItemModels.KSortFilterProxyModel {
        id: filtered
        sourceModel: full.printersModel; sortRoleName: "isClass"
        filterRowCallback: (row, parent) => {
            const index=sourceModel.index(row,0,parent);
            const description=String(sourceModel.data(index,PrintManager.PrinterModel.DestDescription) ?? "");
            const name=String(sourceModel.data(index,PrintManager.PrinterModel.DestName) ?? "");
            return (description + " " + name).toLowerCase().includes(filterString);
        }
    }
    Connections {
        target: PrintManager.PrinterCommands
        function onError(lastError, title, message) { full.actionError = [title,message].filter(Boolean).join("\n"); }
    }
    Connections {
        target: full.printerJobsModel
        function onError(lastError, title, message) { full.actionError = [title,message].filter(Boolean).join("\n"); }
    }
    QtObject {
        id: nativeController
        readonly property var printersModel: filtered
        readonly property bool serverUnavailable: full.serverUnavailable
        readonly property string error: full.actionError || (full.serverUnavailable ? full.printersModelError : "")
        readonly property bool settingsAllowed: KAuthorized.authorizeControlModule("kcm_printer_manager")
        function search(text) { filtered.filterString = text.trim().toLowerCase(); }
        function refresh() { full.actionError=""; full.printersModel.update(); full.printerJobsModel.getJobs(); }
        function settings() { if (settingsAllowed) KCMLauncher.openSystemSettings("kcm_printer_manager"); }
    }
    PrintersView {
        id: view; anchors.fill: parent; controller: nativeController
        printerDelegate: Component { PrinterDelegate { Layout.fillWidth: true; printerJobsModel: full.printerJobsModel; printersModel: full.printersModel } }
    }
}
