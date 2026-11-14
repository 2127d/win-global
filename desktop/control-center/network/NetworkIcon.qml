// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.networkmanagement as PlasmaNM

Kirigami.Icon {
    id: networkIcon
    property int connectionType: PlasmaNM.Enums.UnknownConnectionType
    property string connectionIcon: "network-wireless-symbolic"
    readonly property bool bluetoothNetwork: connectionType === PlasmaNM.Enums.Bluetooth
    source: bluetoothNetwork ? "network-wireless-symbolic" : connectionIcon
    Kirigami.Icon {
        objectName: "networkBluetoothBadge"
        visible: networkIcon.bluetoothNetwork
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: parent.width / 2
        height: width
        source: "preferences-system-bluetooth-symbolic"
    }
}
