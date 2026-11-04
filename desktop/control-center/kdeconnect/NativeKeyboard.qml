// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kdeconnect as KDEConnect
import org.kde.plasma.components as PC
import "../details" as Details

Details.DetailSurface {
    id: root
    required property KDEConnect.DeviceDbusInterface device
    implicitHeight: keyboard.remoteState ? body.implicitHeight + 28 : 0
    visible: keyboard.remoteState
    radius: 22
    ColumnLayout {
        id: body
        anchors.fill: parent; anchors.margins: 14; spacing: 8
        PC.Label { text: qsTr("Remote keyboard"); font.weight: Font.DemiBold }
        KDEConnect.RemoteKeyboard { id: keyboard; Layout.fillWidth: true; device: root.device }
    }
}
