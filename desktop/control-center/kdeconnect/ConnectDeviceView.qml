// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

ColumnLayout {
    id: view
    objectName: "connectDeviceView"
    required property var controller
    property bool notificationsExpanded: false
    property bool commandsExpanded: false
    readonly property var availableActions: controller.actions.filter(action => action.available)
    spacing: 12
    Details.DetailSurface {
        objectName: "connectDeviceCard"
        Layout.fillWidth: true; radius: 22
        implicitHeight: summary.implicitHeight + 28
        emphasized: drop.containsDrag
    DropArea {
        id: drop
        anchors.fill: parent
        enabled: view.controller.reachable && view.controller.shareAvailable
        onEntered: drag => { drag.accepted = drag.hasUrls; }
        onDropped: event => {
            event.accepted = event.hasUrls && view.controller.reachable && view.controller.shareAvailable;
            if (event.accepted) view.controller.sendFiles(event.urls);
        }
    }
        ColumnLayout {
            id: summary; anchors.fill: parent; anchors.margins: 14; spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                Rectangle {
                    Layout.preferredWidth: 42; Layout.preferredHeight: 42; radius: 14
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
                    Kirigami.Icon { anchors.centerIn: parent; width: 26; height: 26; source: view.controller.icon }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 3
                    PC.Label { Layout.fillWidth: true; text: view.controller.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                    PC.Label { text: view.controller.reachable ? qsTr("Connected") : qsTr("Unavailable"); color: Kirigami.Theme.disabledTextColor; font: Kirigami.Theme.smallFont }
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 14
                visible: view.controller.batteryVisible || view.controller.signalVisible
                RowLayout {
                    visible: view.controller.batteryVisible; spacing: 5
                    Kirigami.Icon { Layout.preferredWidth: 18; Layout.preferredHeight: 18; source: view.controller.batteryIcon }
                    PC.Label { objectName: "connectBattery"; text: view.controller.batteryText; font: Kirigami.Theme.smallFont; textFormat: Text.PlainText }
                }
                Item { Layout.fillWidth: true }
                RowLayout {
                    visible: view.controller.signalVisible; spacing: 5
                    Kirigami.Icon { Layout.preferredWidth: 18; Layout.preferredHeight: 18; source: view.controller.signalIcon }
                    PC.Label { text: view.controller.signalText; font: Kirigami.Theme.smallFont; textFormat: Text.PlainText }
                }
            }
            GridLayout {
                Layout.fillWidth: true; columns: width < 300 ? 1 : 2
                columnSpacing: 8; rowSpacing: 8; uniformCellWidths: true
                Repeater {
                    model: view.availableActions
                    delegate: ConnectActionButton {
                        required property var modelData
                        objectName: "connectAction:" + modelData.id
                        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                        implicitHeight: Math.max(42, implicitContentHeight + 14)
                        text: modelData.label; icon.name: modelData.icon
                        checked: modelData.checked ?? false
                        enabled: view.controller.reachable
                        onClicked: view.controller.activate(modelData.id)
                    }
                }
            }
            PC.Label {
                Layout.fillWidth: true; visible: view.controller.shareAvailable
                text: drop.containsDrag ? qsTr("Drop to send to %1").arg(view.controller.name) : qsTr("You can also drop files onto this device.")
                wrapMode: Text.Wrap; textFormat: Text.PlainText; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor
            }
        }
    }
    Loader {
        Layout.fillWidth: true; sourceComponent: view.controller.keyboard
        visible: (item?.implicitHeight ?? 0) > 0
    }
    Details.DetailSurface {
        objectName: "connectNotificationsCard"
        Layout.fillWidth: true; radius: 22
        implicitHeight: notificationBody.implicitHeight + 16
        visible: view.controller.notificationCount > 0
        ColumnLayout {
            id: notificationBody; anchors.fill: parent; anchors.margins: 8; spacing: 8
            RowLayout {
                Layout.fillWidth: true
                ConnectActionButton {
                    objectName: "connectNotificationsToggle"
                    Layout.fillWidth: true; flat: true
                    text: qsTr("Notifications · %1").arg(view.controller.notificationCount)
                    icon.name: view.notificationsExpanded ? "arrow-down-symbolic" : "arrow-right-symbolic"
                    Accessible.name: text; checkable: true; checked: view.notificationsExpanded
                    onClicked: view.notificationsExpanded = !view.notificationsExpanded
                }
                ConnectActionButton {
                    objectName: "connectDismissAll"; flat: true
                    text: qsTr("Dismiss all"); icon.name: "edit-clear-history"
                    display: PC.AbstractButton.IconOnly
                    visible: view.controller.dismissable; enabled: view.controller.reachable
                    onClicked: view.controller.dismissAll()
                    PC.ToolTip { text: parent.text }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 8; visible: view.notificationsExpanded
                Repeater {
                    model: view.controller.notificationModel
                    delegate: ConnectNotification {
                        required property var model
                        Layout.fillWidth: true
                        note: model.modelData ?? model
                        controller: view.controller
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        objectName: "connectCommandsCard"
        Layout.fillWidth: true; radius: 22
        implicitHeight: commandBody.implicitHeight + 16
        visible: view.controller.commandsAvailable && (view.controller.commandCount > 0 || view.controller.canAddCommand)
        ColumnLayout {
            id: commandBody; anchors.fill: parent; anchors.margins: 8; spacing: 8
            RowLayout {
                Layout.fillWidth: true
                ConnectActionButton {
                    objectName: "connectCommandsToggle"
                    Layout.fillWidth: true; flat: true
                    text: qsTr("Remote commands · %1").arg(view.controller.commandCount)
                    icon.name: view.commandsExpanded ? "arrow-down-symbolic" : "arrow-right-symbolic"
                    checkable: true; checked: view.commandsExpanded
                    onClicked: view.commandsExpanded = !view.commandsExpanded
                }
                ConnectActionButton {
                    objectName: "connectAddCommand"; text: qsTr("Add command"); icon.name: "list-add-symbolic"
                    flat: true; display: PC.AbstractButton.IconOnly
                    visible: view.controller.canAddCommand; enabled: view.controller.reachable
                    onClicked: view.controller.addCommand()
                    PC.ToolTip { text: parent.text }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 6; visible: view.commandsExpanded
                PC.Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: qsTr("Add commands on your device to run them here."); visible: view.controller.commandCount === 0; color: Kirigami.Theme.disabledTextColor }
                Repeater {
                    model: view.controller.commandsAvailable ? view.controller.commandModel : null
                    delegate: PC.ItemDelegate {
                        id: command
                        required property var model
                        readonly property var record: model.modelData ?? model
                        objectName: "connectCommand:" + record.key
                        Layout.fillWidth: true
                        implicitHeight: commandLabels.implicitHeight + 20
                        padding: 10; enabled: view.controller.reachable
                        onClicked: view.controller.runCommand(record.key)
                        background: Details.DetailSurface { radius: 14; hovered: command.hovered; focused: command.visualFocus }
                        contentItem: ColumnLayout {
                            id: commandLabels; spacing: 3
                            PC.Label { Layout.fillWidth: true; text: command.record.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.Medium }
                            PC.Label { Layout.fillWidth: true; text: command.record.command; textFormat: Text.PlainText; wrapMode: Text.Wrap; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
                        }
                    }
                }
            }
        }
    }

}
