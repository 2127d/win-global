// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.core as PlasmaCore

// A separate native surface: the catalog is unmapped throughout the wizard.
// Plasma supplies Sequoia's rounded frame, shadow and compositor blur region.
PlasmaCore.Dialog {
    id: dialog
    objectName: "sequoiaPlacementWindow"
    required property var pickerWindow
    readonly property bool opened: GridSession.phase === "destination" || GridSession.phase === "preset"
    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.DialogWindow
    flags: Qt.WindowStaysOnTopHint
    hideOnWindowDeactivate: false
    visible: opened
    x: pickerWindow.x + Math.floor((pickerWindow.width - width) / 2)
    y: pickerWindow.y + Math.floor((pickerWindow.height - height) / 2)
    title: GridSession.phase === "destination" ? qsTr("Add %1").arg(GridSession.title) : qsTr("Choose a layout")
    onOpenedChanged: if (opened) Qt.callLater(function() {
        if (dialog.opened) {
            dialog.requestActivate();
            wizard.forceActiveFocus();
        }
    })
    onClosing: close => {
        close.accepted = false;
        GridSession.cancel();
    }
    mainItem: PlacementWizard {
        id: wizard
        width: dialog.pickerWindow.pickerWidth
        height: dialog.pickerWindow.pickerHeight
    }
}
