// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: card
    required property var device
    property bool spotlight: false
    property var switchDevices: []
    signal deviceChosen(string id)
    signal connectionRequested()
    signal detailsRequested()
    readonly property bool compact: width < 330
    readonly property string status: device.connecting ? qsTr("Connecting…")
        : device.disconnecting ? qsTr("Disconnecting…") : device.failed ? qsTr("Connection failed — try again")
        : device.connected && device.transferring ? qsTr("Transferring files")
        : device.connected && device.playing ? qsTr("Playing audio")
        : device.connected ? qsTr("Connected") : qsTr("Not connected")
    objectName: "bluetoothDevice:" + device.id
    implicitHeight: body.implicitHeight + (spotlight ? 28 : 16)
    radius: spotlight ? 24 : height / 2
    emphasized: false
    ColumnLayout {
        id: body
        anchors.fill: parent
        anchors.margins: card.spotlight ? 14 : 8
        spacing: 14
        RowLayout {
            visible: card.spotlight
            Layout.fillWidth: true
            Rectangle {
                implicitWidth: stateLabel.implicitWidth + 18
                implicitHeight: stateLabel.implicitHeight + 8
                radius: height / 2
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.16)
                PC.Label {
                    id: stateLabel
                    anchors.centerIn: parent
                    text: card.device.audio ? (card.device.connected ? qsTr("Connected audio") : qsTr("Saved audio"))
                        : card.device.connected ? qsTr("Connected device") : qsTr("Saved device")
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9
                }
            }
            Item { Layout.fillWidth: true }
            Flickable {
                // A bounded strip keeps the card usable with many connected devices.
                visible: card.switchDevices.length > 0
                Layout.preferredWidth: Math.min(switchRow.implicitWidth, card.width * 0.46)
                Layout.preferredHeight: 34
                contentWidth: switchRow.implicitWidth
                contentHeight: height
                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                RowLayout {
                    id: switchRow
                    spacing: 2
                    Repeater {
                        model: card.switchDevices.length > 0 ? card.switchDevices : []
                        delegate: BluetoothActionButton {
                            required property var modelData
                            objectName: "bluetoothSwitch:" + modelData.id
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            leftPadding: 7; rightPadding: 7; topPadding: 7; bottomPadding: 7
                            flat: true
                            checked: modelData.id === card.device.id
                            text: modelData.name
                            icon.name: modelData.icon || "network-bluetooth-symbolic"
                            display: PC.AbstractButton.IconOnly
                            Accessible.name: qsTr("Show %1").arg(modelData.name)
                            onClicked: card.deviceChosen(modelData.id)
                            PC.ToolTip { text: parent.text }
                            Rectangle {
                                visible: !!modelData.playing || !!modelData.transferring
                                width: 5; height: 5; radius: 3
                                anchors.right: parent.right; anchors.bottom: parent.bottom
                                anchors.margins: 3
                                color: Kirigami.Theme.highlightColor
                            }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: card.spotlight ? 14 : 9
            Rectangle {
                Layout.preferredWidth: card.spotlight ? (card.compact ? 58 : 76) : 36
                Layout.preferredHeight: width
                radius: card.spotlight ? 24 : width / 2
                color: Qt.alpha(Kirigami.Theme.highlightColor, card.device.connected ? 0.18 : 0.09)
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: card.spotlight ? 40 : 18
                    height: width
                    source: card.device.icon || "network-bluetooth-symbolic"
                    isMask: true
                    color: Kirigami.Theme.textColor
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                PC.Label {
                    Layout.fillWidth: true
                    text: card.device.name
                    textFormat: Text.PlainText
                    font.weight: Font.DemiBold
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * (card.spotlight ? 1.3 : 1)
                    elide: Text.ElideRight
                    wrapMode: card.spotlight ? Text.Wrap : Text.NoWrap
                    maximumLineCount: card.spotlight ? 2 : 1
                    PC.ToolTip { text: card.device.name }
                }
                PC.Label {
                    Layout.fillWidth: true
                    objectName: "bluetoothBattery:" + card.device.id
                    text: card.status + (card.device.battery >= 0 ? " · " + card.device.battery + "%" + (card.spotlight ? qsTr(" battery") : "") : "")
                    textFormat: Text.PlainText
                    color: card.device.failed ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9
                    elide: Text.ElideRight
                }
            }
            PC.BusyIndicator { visible: !!card.device.busy; running: visible; Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
            BluetoothActionButton {
                objectName: "bluetoothConnect:" + card.device.id
                visible: !card.spotlight
                enabled: !card.device.busy
                text: card.device.connected ? qsTr("Disconnect") : qsTr("Connect")
                icon.name: card.device.connected ? "network-disconnect-symbolic" : "network-connect-symbolic"
                display: card.compact ? PC.AbstractButton.IconOnly : PC.AbstractButton.TextOnly
                onClicked: card.connectionRequested()
                PC.ToolTip { text: parent.text }
            }
            BluetoothActionButton {
                flat: true
                objectName: "bluetoothDetails:" + card.device.id
                visible: !card.spotlight
                text: qsTr("Device details")
                icon.name: "go-next-symbolic"
                display: PC.AbstractButton.IconOnly
                onClicked: card.detailsRequested()
                PC.ToolTip { text: parent.text }
            }
        }
        RowLayout {
            visible: card.spotlight
            Layout.fillWidth: true
            spacing: 8
            BluetoothActionButton {
                objectName: "bluetoothSpotlightConnect"
                Layout.fillWidth: true
                enabled: !card.device.busy
                text: card.device.connecting ? qsTr("Connecting…") : card.device.disconnecting ? qsTr("Disconnecting…")
                    : card.device.connected ? qsTr("Disconnect") : qsTr("Connect")
                onClicked: card.connectionRequested()
            }
            BluetoothActionButton {
                objectName: "bluetoothSpotlightDetails"
                Layout.fillWidth: true
                text: qsTr("Details")
                icon.name: "configure-symbolic"
                onClicked: card.detailsRequested()
            }
        }
    }
}
