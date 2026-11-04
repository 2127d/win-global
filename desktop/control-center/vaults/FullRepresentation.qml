// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.extras as PlasmaExtras
PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaDetailPage"
    required property var vaultsModel
    required property var actions
    readonly property real sequoiaPreferredWidth: 418
    readonly property real sequoiaPreferredHeight: view.naturalHeight
    padding: 0; background: null
    KItemModels.KSortFilterProxyModel { id: sorted; sourceModel: root.vaultsModel; sortRoleName: "name" }
    QtObject {
        id: nativeController
        readonly property var vaultsModel: sorted
        function create() { root.actions.requestNewVault(); }
    }
    VaultsView {
        id: view; anchors.fill: parent; controller: nativeController
        vaultDelegate: Component { VaultItem { Layout.fillWidth: true; actions: root.actions } }
    }
}
