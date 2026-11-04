// SPDX-License-Identifier: MIT
// Read-only native model construction; never calls a printer command.
import QtQuick
import org.kde.plasma.printmanager as PrintManager
import ".."
Item {
    id: fixture
    width: 418; height: 600
    PrintManager.PrinterModel { id: printers }
    PrintManager.JobModel { id: jobs }
    PrinterDelegate {
        id: printer; width: 418
        printersModel: printers; printerJobsModel: jobs
        printerState: 5; iconName: "printer"; info: "Private test printer"; location: "Test room"
        stateMessage: "Paused by test"; printerName: "sequoia_private_fixture"
        isDefault: true; isClass: false; index: 0
    }
    FullRepresentation { id: page; visible: false; printersModel: printers; printerJobsModel: jobs; serverUnavailable: true; printersModelError: "Service unavailable" }
    function runChecks() {
        const controller=printer.children.find(child => child.objectName === "printerCard").controller;
        if (!controller.paused || controller.status !== "Paused by test" || !controller.isDefault) throw new Error("Native printer roles not preserved");
        printer.printerState=4;printer.stateMessage="";
        if (!controller.printing || controller.paused || controller.status !== "Printing…") throw new Error("Printing state not reflected");
        printer.isClass=true;
        if (!controller.isGroup) throw new Error("Printer groups not identified");
        jobs.error(1,"Unable to cancel","The job is no longer available");
        if (page.actionError !== "Unable to cancel\nThe job is no longer available") throw new Error("Native job error was lost");
        return true;
    }
}
