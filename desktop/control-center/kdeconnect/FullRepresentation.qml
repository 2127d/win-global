// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kdeconnect as KDEConnect
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaDetailPage"
    property var devicesModel
    readonly property real sequoiaPreferredWidth: 418
    readonly property real sequoiaPreferredHeight: view.naturalHeight
    padding: 0
    background: null
    KDEConnect.DevicesModel { id: allDevices }
    KDEConnect.DevicesModel { id: pairedDevices; displayFilter: KDEConnect.DevicesModel.Paired }
    ConnectView {
        id: view; anchors.fill: parent
        devicesModel: root.devicesModel
        pairedCount: pairedDevices.count; discoveredCount: allDevices.count
        deviceDelegate: Component { DeviceDelegate { Layout.fillWidth: true } }
        onSettingsRequested: KDEConnect.OpenConfig.openConfiguration()
        onStoreRequested: url => Qt.openUrlExternally(url)
    }
}
