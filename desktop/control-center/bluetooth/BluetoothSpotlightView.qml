// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details
import "BluetoothDevices.js" as Devices

FocusScope {
    id: view
    objectName: "bluetoothSpotlightView"
    required property var controller
    property bool showPowerControl: true
    property Component mediaComponent: null
    property string selectedId: ""
    readonly property var devices: controller.devices
    property var selectionState: ({order: [], activeIds: [], featuredId: "", manualId: ""})
    readonly property var connectionOrder: controller.connectionOrder ?? []
    readonly property var featured: devices.find(d => d.id === selectionState.featuredId) || null
    readonly property var switchDevices: selectionState.activeIds.map(id => devices.find(d => d.id === id)).filter(d => !!d && Devices.active(d))
    function refreshSelection() {
        selectionState = Devices.selection(devices, selectionState, connectionOrder);
    }
    function chooseDevice(id) {
        if (!switchDevices.some(d => d.id === id)) return;
        selectionState = Object.assign({}, selectionState, {featuredId: id, manualId: id});
    }
    onDevicesChanged: Qt.callLater(refreshSelection)
    onConnectionOrderChanged: Qt.callLater(refreshSelection)
    Component.onCompleted: refreshSelection()
    readonly property var remaining: devices.filter(d => d.id !== featured?.id)
    readonly property var selectedDevice: devices.find(d => d.id === selectedId) || null
    readonly property int navigationDepth: selectedDevice ? 2 : 1
    function goBack() { selectedId = ""; scroll.contentItem.contentY = 0; }
    function openDetails(id) { selectedId = id; scroll.contentItem.contentY = 0; }
    onSelectedDeviceChanged: if (selectedId && !selectedDevice) Qt.callLater(function() {
        if (view.selectedId && !view.selectedDevice) view.goBack();
    })
    Connections { target: view.controller; function onOperationalChanged() { if (!view.controller.operational) view.goBack(); } }
    Keys.onEscapePressed: event => { if (selectedId) { goBack(); event.accepted = true; } else event.accepted = false; }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Details.DetailSurface {
            visible: view.showPowerControl
            Layout.fillWidth: true
            implicitHeight: powerRow.implicitHeight + 8
            radius: height / 2
            RowLayout {
                id: powerRow
                anchors.fill: parent; anchors.margins: 4
                PC.Label { Layout.fillWidth: true; Layout.leftMargin: 10; text: qsTr("Bluetooth"); font.weight: Font.DemiBold }
                PC.Switch {
                    objectName: "bluetoothPowerSwitch"
                    Accessible.name: qsTr("Bluetooth")
                    checked: view.controller.operational
                    enabled: view.controller.adapterAvailable
                    onToggled: view.controller.setEnabled(checked)
                }
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            PC.ScrollView {
                id: scroll
                objectName: "bluetoothDeviceScroll"
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                // Keep device surfaces aligned with the header; reserve the
                // extra viewport width only for their soft outer shadows.
                anchors.leftMargin: -6; anchors.rightMargin: -6
                anchors.bottom: island.top; anchors.bottomMargin: -8
                background: null
                PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
                PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
                layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                layer.effect: MultiEffect { maskEnabled: true; maskSource: edgeMask; maskThresholdMin: 0.5; maskSpreadAtMin: 1; autoPaddingEnabled: false }
                contentItem: Flickable {
                    clip: true
                    contentWidth: width
                    contentHeight: contents.height + 32
                    boundsBehavior: Flickable.StopAtBounds
                    ColumnLayout {
                        id: contents
                        x: 6; y: 6
                        width: Math.max(0, scroll.availableWidth - 12)
                        spacing: 10
                        ColumnLayout {
                            visible: !view.controller.operational || !view.devices.length
                            Layout.fillWidth: true
                            Layout.topMargin: 45
                            spacing: 14
                            Kirigami.Icon { source: "network-bluetooth-symbolic"; Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 48; Layout.preferredHeight: 48 }
                            PC.Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
                                text: !view.controller.adapterAvailable ? qsTr("No Bluetooth adapters available")
                                    : !view.controller.operational ? qsTr("Bluetooth is off") : qsTr("No devices paired")
                            }
                            BluetoothActionButton {
                                objectName: "bluetoothEmptyAction"
                                Layout.alignment: Qt.AlignHCenter
                                visible: view.controller.adapterAvailable
                                text: view.controller.operational ? qsTr("Pair new device") : qsTr("Turn on Bluetooth")
                                onClicked: view.controller.operational ? view.controller.pair() : view.controller.setEnabled(true)
                            }
                        }
                        ColumnLayout {
                            visible: view.controller.operational && !view.selectedDevice && !!view.featured
                            Layout.fillWidth: true
                            spacing: 10
                            BluetoothDeviceCard {
                                Layout.fillWidth: true
                                spotlight: true
                                switchDevices: view.switchDevices
                                onDeviceChosen: id => view.chooseDevice(id)
                                device: view.featured || {id:"",name:"",battery:-1}
                                onConnectionRequested: view.controller.toggleDevice(device.id)
                                onDetailsRequested: view.openDetails(device.id)
                            }
                            PC.Label {
                                visible: view.remaining.length > 0
                                Layout.leftMargin: 10; Layout.topMargin: 4
                                text: qsTr("Your devices")
                                color: Kirigami.Theme.disabledTextColor
                            }
                            Repeater {
                                model: view.remaining
                                delegate: BluetoothDeviceCard {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    device: modelData
                                    onConnectionRequested: view.controller.toggleDevice(device.id)
                                    onDetailsRequested: view.openDetails(device.id)
                                }
                            }
                        }
                        Details.DetailSurface {
                            visible: view.controller.operational && !!view.selectedDevice
                            Layout.fillWidth: true
                            implicitHeight: detailColumn.implicitHeight + 28
                            radius: 24
                            ColumnLayout {
                                id: detailColumn
                                anchors.fill: parent; anchors.margins: 14
                                spacing: 12
                                BluetoothActionButton {
                                    flat: true
                                    objectName: "bluetoothDetailsBack"
                                    text: qsTr("Your devices"); icon.name: "go-previous-symbolic"
                                    onClicked: view.goBack()
                                }
                                PC.Label { Layout.fillWidth: true; text: view.selectedDevice?.name ?? ""; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2 }
                                PC.Label { Layout.fillWidth: true; text: view.selectedDevice?.failed ? qsTr("Connection failed — try again") : view.selectedDevice?.connecting ? qsTr("Connecting…") : view.selectedDevice?.disconnecting ? qsTr("Disconnecting…") : view.selectedDevice?.connected ? qsTr("Connected") : qsTr("Not connected"); wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
                                GridLayout {
                                    Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 10
                                    Repeater {
                                        model: view.selectedDevice ? [qsTr("Address"), view.selectedDevice.address, qsTr("Paired"), view.selectedDevice.paired ? qsTr("Yes") : qsTr("No"), qsTr("Trusted"), view.selectedDevice.trusted ? qsTr("Yes") : qsTr("No"), qsTr("Adapter"), view.selectedDevice.adapterName, ...(view.selectedDevice.battery >= 0 ? [qsTr("Battery"), view.selectedDevice.battery + "%"] : [])] : []
                                        delegate: PC.Label {
                                            required property int index
                                            required property string modelData
                                            text: modelData
                                            textFormat: Text.PlainText
                                            Layout.fillWidth: index % 2 === 1
                                            wrapMode: Text.WrapAnywhere
                                            color: index % 2 ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                                        }
                                    }
                                }
                                BluetoothActionButton {
                                    objectName: "bluetoothDetailConnect"
                                    Layout.fillWidth: true
                                    enabled: !view.selectedDevice?.busy
                                    text: view.selectedDevice?.connected ? qsTr("Disconnect") : qsTr("Connect")
                                    onClicked: view.controller.toggleDevice(view.selectedId)
                                }
                                Loader {
                                    Layout.fillWidth: true
                                    property var deviceRecord: view.selectedDevice
                                    active: !!view.selectedDevice?.mediaPlayer && !!view.mediaComponent
                                    sourceComponent: view.mediaComponent
                                }
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    BluetoothActionButton { text: qsTr("Copy address"); onClicked: view.controller.copyAddress(view.selectedId) }
                                    BluetoothActionButton { visible: view.selectedDevice?.canBrowse ?? false; text: qsTr("Browse files"); onClicked: view.controller.browse(view.selectedId) }
                                    BluetoothActionButton { visible: view.selectedDevice?.canSend ?? false; text: qsTr("Send file"); onClicked: view.controller.send(view.selectedId) }
                                    BluetoothActionButton { objectName: "bluetoothForget"; text: qsTr("Forget device…"); enabled: !view.selectedDevice?.busy; onClicked: view.controller.forget(view.selectedId) }
                                }
                            }
                        }
                    }
                }
            }
            Rectangle {
                id: edgeMask
                width: scroll.width; height: scroll.height
                visible: false; layer.enabled: true
                readonly property real fade: Math.min(24, height / 3) / Math.max(1, height)
                gradient: Gradient {
                    GradientStop { position: 0; color: scroll.contentItem.contentY > 1 ? "transparent" : "white" }
                    GradientStop { position: edgeMask.fade; color: "white" }
                    GradientStop { position: 1 - edgeMask.fade; color: "white" }
                    GradientStop { position: 1; color: "transparent" }
                }
            }
            Details.DetailSurface {
                id: island
                objectName: "bluetoothActionIsland"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                width: Math.min(parent.width - 12, islandRow.implicitWidth + 20)
                height: islandRow.implicitHeight + 10
                radius: height / 2
                elevated: true
                color: Qt.alpha(stateColor, dark ? 0.94 : 0.90)
                z: 5
                RowLayout {
                    id: islandRow
                    anchors.fill: parent; anchors.margins: 5
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 4
                    BluetoothActionButton {
                        flat: true
                        objectName: "bluetoothPair"
                        enabled: view.controller.operational
                        text: qsTr("Pair new device")
                        icon.name: "list-add-symbolic"
                        onClicked: view.controller.pair()
                    }
                    Rectangle { Layout.preferredHeight: 18; Layout.preferredWidth: 1; color: Qt.alpha(Kirigami.Theme.textColor, 0.15) }
                    BluetoothActionButton {
                        flat: true
                        objectName: "bluetoothSettings"
                        text: qsTr("Bluetooth settings")
                        icon.name: "configure-symbolic"
                        display: PC.AbstractButton.IconOnly
                        onClicked: view.controller.settings()
                        PC.ToolTip { text: parent.text }
                    }
                }
            }
        }
    }
}
