// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.Representation {
    id: root
    objectName: "sequoiaDetailPage"
    required property var controller
    readonly property real sequoiaPreferredWidth: 418
    readonly property real sequoiaPreferredHeight: view.naturalHeight
    padding: 0
    background: null
    DisplayConfigurationView { id: view; anchors.fill: parent; controller: root.controller }
}
