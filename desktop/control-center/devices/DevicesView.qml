// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    objectName: "disksDevicesView"
    required property var controller
    required property Component deviceDelegate
    readonly property real naturalHeight: filters.implicitHeight + content.implicitHeight + footer.implicitHeight + 32
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    Connections {
        target: view.controller
        function onFilterChanged() { contentReveal.restart(); }
    }
    NumberAnimation {
        id: contentReveal
        target: content; property: "opacity"; from: 0.25; to: 1
        duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic
    }
    RowLayout {
        id: filters; objectName: "diskFilters"
        anchors { top: parent.top; left: parent.left; right: parent.right }
        spacing: 6
        Repeater {
            model: [{id:"removable",label:qsTr("Removable")},{id:"internal",label:qsTr("Internal")},{id:"all",label:qsTr("All devices")}]
            delegate: DeviceActionButton {
                required property var modelData
                objectName: "diskFilter:" + modelData.id
                Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0
                text: modelData.label; checkable: true; checked: view.controller.filter === modelData.id
                onClicked: view.controller.setFilter(modelData.id)
            }
        }
    }
    PC.ScrollView {
        anchors { top: filters.bottom; topMargin: 12; left: parent.left; right: parent.right; bottom: footer.top; bottomMargin: 12 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true
            contentWidth: width; contentHeight: content.implicitHeight + 8
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content; width: parent.width; spacing: 12
                Repeater { id: devices; model: view.controller.devicesModel; delegate: view.deviceDelegate }
                Details.DetailSurface {
                    objectName: "diskEmptyCard"
                    Layout.fillWidth: true; radius: 22
                    visible: devices.count === 0
                    implicitHeight: emptyBody.implicitHeight + 32
                    ColumnLayout {
                        id: emptyBody; anchors.fill: parent; anchors.margins: 16; spacing: 12
                        Kirigami.Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 40; Layout.preferredHeight: 40; source: "drive-removable-media-symbolic" }
                        PC.Label {
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; font.weight: Font.DemiBold
                            text: view.controller.filter === "removable" ? qsTr("No removable devices attached") : qsTr("No disks available")
                        }
                        PC.Label {
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                            text: view.controller.filter === "removable" ? qsTr("Connect a drive, memory card or other storage device to get started.") : qsTr("Available storage devices will appear here.")
                        }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer; objectName: "diskFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, footerActions.implicitWidth + 16)
        implicitHeight: footerActions.implicitHeight + 8
        radius: height / 2; elevated: true
        RowLayout {
            id: footerActions; anchors.centerIn: parent; spacing: 4
            DeviceActionButton {
                objectName: "diskSettings"; flat: true; visible: view.controller.settingsAllowed
                text: qsTr("Settings"); icon.name: "configure-symbolic"
                onClicked: view.controller.settings()
            }
            DeviceActionButton {
                objectName: "diskRemoveAll"; flat: true; visible: view.controller.unmountableCount > 0
                text: qsTr("Remove all"); icon.name: "media-eject-symbolic"
                Accessible.description: qsTr("Safely remove all removable devices")
                onClicked: view.controller.removeAll()
            }
            DeviceActionButton {
                objectName: "diskAutoPopup"; flat: true
                text: qsTr("Show when a device is connected"); icon.name: "notifications-symbolic"
                display: PC.AbstractButton.IconOnly; checkable: true; checked: view.controller.autoPopup
                Accessible.name: text
                onClicked: view.controller.setAutoPopup(!view.controller.autoPopup)
                PC.ToolTip { text: parent.text }
            }
        }
    }
}
