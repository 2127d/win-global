// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    objectName: "vaultCard"
    required property var controller
    implicitHeight: body.implicitHeight + 28
    radius: 22
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
                PC.Label { Layout.fillWidth: true; text: card.controller.name; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                PC.Label { objectName: "vaultStatus"; Layout.fillWidth: true; text: card.controller.busy ? qsTr("Working…") : !card.controller.available ? qsTr("Unavailable") : card.controller.opened ? qsTr("Unlocked") : qsTr("Locked"); wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor; font: Kirigami.Theme.smallFont }
            }
            PC.BusyIndicator { objectName: "vaultBusy"; Layout.preferredWidth: 24; Layout.preferredHeight: 24; visible: card.controller.busy; running: visible }
        }
        PC.Label {
            objectName: "vaultOffline"
            Layout.fillWidth: true; visible: card.controller.offlineOnly
            text: qsTr("Offline-only vault"); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor
        }
        PC.Label {
            objectName: "vaultError"
            Layout.fillWidth: true; visible: text.length > 0
            text: card.controller.message; textFormat: Text.PlainText; wrapMode: Text.Wrap
            color: Kirigami.Theme.negativeTextColor
        }
        GridLayout {
            Layout.fillWidth: true; columns: width < 300 ? 1 : 2; uniformCellWidths: true; columnSpacing: 8; rowSpacing: 8
            VaultActionButton {
                objectName: "vaultPrimary"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0
                text: card.controller.opened ? qsTr("Lock vault") : qsTr("Unlock and open")
                icon.name: card.controller.opened ? "lock-symbolic" : "unlock-symbolic"
                enabled: card.controller.canAct
                onClicked: card.controller.primary()
            }
            VaultActionButton {
                objectName: "vaultSecondary"
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0
                text: card.controller.opened ? qsTr("Open files") : qsTr("Unlock only")
                icon.name: card.controller.opened ? "document-open-folder-symbolic" : "unlock-symbolic"
                enabled: card.controller.canAct
                onClicked: card.controller.secondary()
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            VaultActionButton {
                objectName: "vaultConfigure"
                Layout.fillWidth: true; Layout.preferredWidth: 1; flat: true
                text: qsTr("Configure"); icon.name: "configure-symbolic"; enabled: card.controller.canAct
                onClicked: card.controller.configure()
            }
            VaultActionButton {
                objectName: "vaultForceLock"
                Layout.fillWidth: true; Layout.preferredWidth: 1; flat: true
                text: qsTr("Force lock"); icon.name: "window-close-symbolic"
                visible: card.controller.opened; enabled: card.controller.canAct
                onClicked: card.controller.forceLock()
            }
        }
    }
}
