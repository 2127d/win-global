// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    objectName: "printerCard"
    required property var controller
    property bool jobsExpanded: controller.activeCount > 0
    radius: 22; implicitHeight: body.implicitHeight + 28
    ColumnLayout {
        id: body; anchors.fill: parent; anchors.margins: 14; spacing: 12
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Rectangle {
                Layout.preferredWidth: 42; Layout.preferredHeight: 42; radius: 14
                color: Qt.alpha(Kirigami.Theme.highlightColor,0.12)
                Kirigami.Icon { anchors.centerIn: parent; width: 28; height: 28; source: card.controller.icon }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                PC.Label { Layout.fillWidth: true; text: card.controller.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                PC.Label { Layout.fillWidth: true; text: card.controller.location; visible: text.length > 0; textFormat: Text.PlainText; wrapMode: Text.Wrap; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
                PC.Label { text: card.controller.isGroup ? qsTr("Printer group") : qsTr("Default printer"); visible: card.controller.isGroup || card.controller.isDefault; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
            }
            PC.BusyIndicator { Layout.preferredWidth: 22; Layout.preferredHeight: 22; visible: card.controller.printing; running: visible }
        }
        PC.Label {
            objectName: "printerStatus"
            Layout.fillWidth: true; text: card.controller.status; textFormat: Text.PlainText; wrapMode: Text.Wrap
            color: card.controller.paused ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.disabledTextColor
        }
        GridLayout {
            Layout.fillWidth: true; columns: width < 300 ? 1 : 2; columnSpacing: 8; rowSpacing: 8; uniformCellWidths: true
            PrinterActionButton { objectName: "printerQueue"; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0; text: qsTr("Print queue"); icon.name: "view-list-details"; enabled: card.controller.available; onClicked: card.controller.openQueue() }
            PrinterActionButton { objectName: "printerPause"; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0; text: card.controller.paused ? qsTr("Resume") : qsTr("Pause"); icon.name: card.controller.paused ? "media-playback-start" : "media-playback-pause"; enabled: card.controller.available; onClicked: card.controller.togglePaused() }
        }
        RowLayout {
            Layout.fillWidth: true
            PrinterActionButton {
                objectName: "printerJobsToggle"; Layout.fillWidth: true; flat: true
                text: card.controller.jobCount > 0 ? qsTr("Jobs · %1").arg(card.controller.jobCount) : qsTr("Queue is empty")
                icon.name: card.jobsExpanded ? "arrow-down-symbolic" : "arrow-right-symbolic"
                enabled: card.controller.jobCount > 0; checkable: true; checked: card.jobsExpanded
                onClicked: card.jobsExpanded = !card.jobsExpanded
            }
            PrinterActionButton {
                objectName: "printerConfigure"; flat: true; text: qsTr("Configure printer"); icon.name: "configure-symbolic"
                display: PC.AbstractButton.IconOnly; visible: card.controller.settingsAllowed
                onClicked: card.controller.configure()
                PC.ToolTip { text: parent.text }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: card.jobsExpanded && card.controller.jobCount > 0; spacing: 8
            Repeater {
                model: card.controller.jobsModel
                delegate: Details.DetailSurface {
                    required property var model
                    readonly property var record: model.modelData ?? model
                    objectName: "printerJob"
                    Layout.fillWidth: true; implicitHeight: jobRow.implicitHeight + 16; radius: 14
                    RowLayout {
                        id: jobRow; anchors.fill: parent; anchors.margins: 8; spacing: 8
                        Kirigami.Icon { Layout.preferredWidth: 20; Layout.preferredHeight: 20; source: record.jobIconName }
                        PC.Label { Layout.fillWidth: true; text: record.jobName; textFormat: Text.PlainText; wrapMode: Text.Wrap }
                        PrinterActionButton {
                            objectName: "printerCancelJob:" + record.jobId
                            flat: true; text: qsTr("Cancel job"); icon.name: "dialog-cancel-symbolic"; display: PC.AbstractButton.IconOnly
                            visible: record.jobCancelEnabled; enabled: card.controller.available
                            onClicked: card.controller.cancelJob(record)
                            PC.ToolTip { text: parent.text }
                        }
                    }
                }
            }
            PrinterActionButton {
                objectName: "printerMoreJobs"; Layout.fillWidth: true; flat: true
                visible: card.controller.jobCount > 3; text: qsTr("View all %1 jobs…").arg(card.controller.jobCount)
                icon.name: "view-more-symbolic"; enabled: card.controller.available; onClicked: card.controller.openQueue()
            }
        }
    }
}
