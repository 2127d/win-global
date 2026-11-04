// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    required property var controller
    property Component batteryCards
    property Component inhibitionDetails
    objectName: "powerView"
    implicitWidth: 418
    implicitHeight: content.implicitHeight + island.implicitHeight + 36
    PC.ScrollView {
        id: scroll
        objectName: "powerScroll"
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: island.top; bottomMargin: 10 }
        background: null
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true
            contentWidth: width; contentHeight: content.implicitHeight + 12
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content
                x: 6; y: 6; width: Math.max(0, scroll.availableWidth - 12)
                spacing: 12
                Loader { Layout.fillWidth: true; active: !!view.batteryCards; visible: active && implicitHeight > 0; sourceComponent: view.batteryCards }
                PowerProfileCard { Layout.fillWidth: true; controller: view.controller }
                Details.DetailSurface {
                    objectName: "powerSleepCard"
                    Layout.fillWidth: true; radius: 24
                    implicitHeight: sleepContent.implicitHeight + 28
                    ColumnLayout {
                        id: sleepContent
                        anchors.fill: parent; anchors.margins: 14; spacing: 10
                        RowLayout {
                            spacing: 12
                            Rectangle {
                                Layout.preferredWidth: 38; Layout.preferredHeight: 38
                                radius: 19; color: Qt.alpha(Kirigami.Theme.highlightColor, view.controller.isManuallyInhibited ? 0.22 : 0.09)
                                Kirigami.Icon { anchors.centerIn: parent; width: 20; height: 20; source: view.controller.isManuallyInhibited ? "system-suspend-inhibited" : "system-suspend-uninhibited" }
                            }
                            PC.Label { Layout.fillWidth: true; text: qsTr("Keep awake"); font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                            PC.Switch {
                                id: awakeSwitch
                                objectName: "powerKeepAwake"
                                checked: view.controller.isManuallyInhibited
                                Accessible.name: qsTr("Keep awake")
                                Accessible.description: qsTr("Block automatic sleep and screen locking")
                                onToggled: {
                                    view.controller.setInhibited(!view.controller.isManuallyInhibited);
                                    checked = Qt.binding(() => view.controller.isManuallyInhibited);
                                }
                            }
                        }
                        PC.Label {
                            Layout.fillWidth: true
                            objectName: "powerSleepDescription"
                            text: view.controller.isManuallyInhibited
                                ? qsTr("Automatic sleep and screen locking are blocked. This uses more energy.")
                                : qsTr("Prevent automatic sleep and screen locking while you need the computer awake.")
                            wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                        }
                        Loader {
                            Layout.fillWidth: true
                            active: view.controller.requestedInhibitions.length > 0 || view.controller.inhibitsLidAction
                            visible: active
                            sourceComponent: view.inhibitionDetails
                        }
                    }
                }
                PC.Label { objectName: "powerError"; Layout.fillWidth: true; Layout.leftMargin: 10; Layout.rightMargin: 10; visible: text.length > 0; text: view.controller.errorMessage; wrapMode: Text.Wrap; color: Kirigami.Theme.negativeTextColor; Accessible.role: Accessible.AlertMessage }
            }
        }
    }
    Details.DetailSurface {
        id: island
        objectName: "powerActionIsland"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, actions.implicitWidth + 16)
        implicitHeight: actions.implicitHeight + 8
        radius: height / 2; elevated: true
        RowLayout {
            id: actions
            anchors.centerIn: parent
            spacing: 2
            PowerActionButton { objectName: "powerSettings"; visible: view.controller.settingsAllowed; flat: true; text: qsTr("Settings"); icon.name: "configure-symbolic"; onClicked: view.controller.settings() }
            Rectangle { visible: view.controller.settingsAllowed && view.controller.energyAllowed; width: 1; height: 18; color: Qt.alpha(Kirigami.Theme.textColor, 0.15) }
            PowerActionButton { objectName: "powerEnergy"; visible: view.controller.energyAllowed; flat: true; text: qsTr("Energy info"); icon.name: "documentinfo"; onClicked: view.controller.energy() }
        }
        visible: view.controller.settingsAllowed || view.controller.energyAllowed
    }
}
