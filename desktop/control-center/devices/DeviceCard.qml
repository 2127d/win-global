// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    objectName: "diskDeviceCard"
    required property var controller
    property bool toolsExpanded: false
    property bool attention: false
    implicitHeight: body.implicitHeight + 28
    radius: 22; emphasized: attention
    ColumnLayout {
        id: body; anchors.fill: parent; anchors.margins: 14; spacing: 12
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Rectangle {
                Layout.preferredWidth: 44; Layout.preferredHeight: 44; radius: 14
                color: Qt.alpha(Kirigami.Theme.highlightColor,0.12)
                Kirigami.Icon { anchors.centerIn: parent; width: 30; height: 30; source: card.controller.icon }
                Kirigami.Icon { anchors.right: parent.right; anchors.bottom: parent.bottom; width: 18; height: 18; source: card.controller.emblem; visible: !!source }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 3
                PC.Label { objectName: "diskName"; Layout.fillWidth: true; text: card.controller.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                PC.Label { Layout.fillWidth: true; text: card.controller.kind; textFormat: Text.PlainText; wrapMode: Text.Wrap; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
            }
            PC.BusyIndicator { objectName: "diskBusy"; Layout.preferredWidth: 24; Layout.preferredHeight: 24; visible: card.controller.busy; running: visible }
        }
        PC.Label {
            objectName: "diskStatus"
            Layout.fillWidth: true; text: card.controller.status; textFormat: Text.PlainText; wrapMode: Text.Wrap
            color: card.controller.failed ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
        }
        ColumnLayout {
            Layout.fillWidth: true; visible: card.controller.capacityVisible; spacing: 6
            Rectangle {
                objectName: "diskCapacity"
                Layout.fillWidth: true; implicitHeight: 6; radius: 3
                color: Qt.alpha(Kirigami.Theme.textColor,0.1)
                Rectangle {
                    width: parent.width * Math.max(0,Math.min(1,card.controller.usedFraction)); height: parent.height; radius: 3
                    anchors.right: Qt.application.layoutDirection === Qt.RightToLeft ? parent.right : undefined
                    color: card.controller.lowSpace ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor
                }
                Accessible.role: Accessible.ProgressBar
                Accessible.name: card.controller.capacityText
            }
            PC.Label {
                objectName: "diskCapacityText"
                Layout.fillWidth: true; text: card.controller.capacityText; wrapMode: Text.Wrap; font: Kirigami.Theme.smallFont
                color: card.controller.lowSpace ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
            }
        }
        GridLayout {
            id: actionGrid
            Layout.fillWidth: true
            columns: width < 300 ? 1 : 2; columnSpacing: 8; rowSpacing: 8
            uniformCellWidths: true
            DeviceActionButton {
                objectName: "diskDefaultAction"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                Layout.columnSpan: actions.count === 0 ? actionGrid.columns : 1
                visible: card.controller.defaultText.length > 0
                text: card.controller.defaultText; icon.name: card.controller.defaultIcon
                enabled: card.controller.canAct
                onClicked: card.controller.triggerDefault()
            }
            Repeater {
                id: actions
                model: card.controller.actionsModel
                delegate: DeviceActionButton {
                    required property var model
                    required property int index
                    readonly property var record: model.modelData ?? model
                    objectName: "diskAction:" + record.Name
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                    Layout.columnSpan: index > 0 || card.controller.defaultText.length === 0 ? actionGrid.columns : 1
                    visible: index === 0 || card.toolsExpanded
                    enabled: card.controller.canAct
                    text: record.Text; icon.name: record.Icon
                    onClicked: card.controller.triggerAction(record.Name)
                }
            }
        }
        DeviceActionButton {
            objectName: "diskMoreActions"
            Layout.fillWidth: true; flat: true; visible: actions.count > 1
            text: card.toolsExpanded ? qsTr("Fewer actions") : qsTr("More actions · %1").arg(actions.count - 1)
            icon.name: card.toolsExpanded ? "arrow-up-symbolic" : "arrow-down-symbolic"
            checkable: true; checked: card.toolsExpanded
            onClicked: card.toolsExpanded = !card.toolsExpanded
        }
    }
}
