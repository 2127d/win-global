// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    required property var controller
    objectName: "powerModeCard"
    radius: 24
    implicitHeight: content.implicitHeight + 32
    readonly property var choices: [
        {id: "power-saver", name: qsTr("Power Save"), icon: "battery-profile-powersave-symbolic", description: qsTr("Use less energy for everyday tasks.")},
        {id: "balanced", name: qsTr("Balanced"), icon: "speedometer", description: qsTr("Balance performance and energy use.")},
        {id: "performance", name: qsTr("Performance"), icon: "battery-profile-performance-symbolic", description: qsTr("Prioritize speed for demanding tasks.")}
    ]
    readonly property var current: choices.find(p => p.id === controller.activeProfile)
    readonly property bool available: controller.profiles.length > 0
    readonly property string modeName: available ? (current?.name || controller.activeProfile) : controller.isTlpInstalled ? qsTr("Automatic") : qsTr("Unavailable")
    readonly property string description: !available
        ? controller.isTlpInstalled ? qsTr("TLP manages power automatically.")
        : controller.profilesInstalled ? qsTr("This device does not offer power modes.") : qsTr("The power profile service is unavailable.")
        : current?.description || qsTr("Managed by your system.")
    readonly property string constraint: {
        const limited = controller.inhibitionReason;
        const reduced = controller.activeProfile === "performance" ? controller.degradationReason : "";
        if (!limited && !reduced) return "";
        const reason = limited || reduced;
        if (reason === "lap-detected") return limited ? qsTr("Performance is unavailable while the computer is on your lap.") : qsTr("Performance is reduced while the computer is on your lap.");
        if (reason === "high-operating-temperature") return limited ? qsTr("Performance is unavailable while the computer cools down.") : qsTr("Performance is reduced while the computer cools down.");
        return limited ? qsTr("Performance is currently unavailable.") : qsTr("Performance is currently reduced.");
    }
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 16
        spacing: 16
        PC.Label { text: qsTr("Power mode"); color: Kirigami.Theme.disabledTextColor; font: Kirigami.Theme.smallFont }
        RowLayout {
            spacing: 14
            Rectangle {
                Layout.preferredWidth: 56; Layout.preferredHeight: 56
                radius: 18; color: Qt.alpha(Kirigami.Theme.highlightColor, 0.14)
                Kirigami.Icon { anchors.centerIn: parent; width: 28; height: 28; source: card.current?.icon || "speedometer"; isMask: true }
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 4
                PC.Label { objectName: "powerModeTitle"; Layout.fillWidth: true; text: card.modeName; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.25; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                PC.Label { Layout.fillWidth: true; text: card.description; color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap }
            }
        }
        GridLayout {
            Layout.fillWidth: true
            visible: card.available
            columns: width < 340 || Kirigami.Theme.defaultFont.pointSize > 12 ? 1 : 3
            columnSpacing: 6; rowSpacing: 6
            Repeater {
                model: card.choices
                PowerActionButton {
                    required property var modelData
                    objectName: "powerProfile:" + modelData.id
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                    text: modelData.name
                    checkable: true
                    checked: card.controller.activeProfile === modelData.id
                    enabled: card.controller.profiles.includes(modelData.id) && !(modelData.id === "performance" && card.controller.inhibitionReason !== "")
                    Accessible.description: !card.controller.profiles.includes(modelData.id) ? qsTr("Not supported on this device") : modelData.id === "performance" && card.controller.inhibitionReason ? card.constraint : modelData.description
                    // Only the backend selects a mode; failures must never leave a false checkmark.
                    onClicked: {
                        if (card.controller.activeProfile !== modelData.id) card.controller.activateProfile(modelData.id);
                        checked = Qt.binding(() => card.controller.activeProfile === modelData.id);
                    }
                    PC.ToolTip { text: parent.Accessible.description; visible: parent.hovered; delay: Kirigami.Units.toolTipDelay }
                }
            }
        }
        PC.Label { objectName: "powerConstraint"; Layout.fillWidth: true; visible: text.length > 0; text: card.constraint; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
        Repeater {
            model: card.controller.profileHolds.filter(h => h.profile === card.controller.activeProfile)
            RowLayout {
                required property var modelData
                Layout.fillWidth: true; spacing: 8
                Kirigami.Icon { Layout.preferredWidth: 18; Layout.preferredHeight: 18; source: modelData.icon || "application-x-executable" }
                PC.Label { Layout.fillWidth: true; text: qsTr("Requested by %1: %2").arg(modelData.name).arg(modelData.reason); textFormat: Text.PlainText; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
            }
        }
    }
}
