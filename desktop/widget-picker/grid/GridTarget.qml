// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.core as PlasmaCore

Item {
    id: target
    required property var containment
    required property var view
    property string kind: "panel"
    readonly property int screen: containment?.plasmoid?.screen ?? -1
    readonly property int location: containment?.plasmoid?.location ?? 0
    readonly property rect geometry: Qt.rect(view.x, view.y, view.width, view.height)
    readonly property bool ready: containment !== null && screen >= 0
        && containment.plasmoid.immutability === PlasmaCore.Types.Mutable
    readonly property string label: qsTr("%1 panel · Screen %2").arg(
        location === PlasmaCore.Types.TopEdge ? qsTr("Top") :
        location === PlasmaCore.Types.LeftEdge ? qsTr("Left") :
        location === PlasmaCore.Types.RightEdge ? qsTr("Right") : qsTr("Bottom")).arg(screen + 1)
    function addPanelWidget(plugin) {
        if (!ready) return false;
        const result = GridSession.addApplet(containment, plugin);
        if (!result || result.failedToLaunch) {
            GridSession.error = qsTr("The widget could not be added to this panel.");
            return false;
        }
        return true;
    }
    Component.onCompleted: GridSession.registerTarget(target)
    Component.onDestruction: GridSession.unregisterTarget(target)
}
