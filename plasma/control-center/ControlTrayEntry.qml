// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PlasmaExtras

ControlTile {
    id: card
    required property var entry
    signal openApplet(string plugin)
    objectName: "trayCard:" + entry.id
    title: entry.applet?.Plasmoid.title || entry.title || entry.id
    subtitle: (entry.applet?.toolTipSubText || entry.subtitle || qsTr("Open")).replace(/<[^>]*>/g, " ").replace(/\s+/g, " ").trim()
    symbol: entry.applet?.Plasmoid.icon || entry.icon || "application-x-executable"
    maskIcon: false
    checkable: false
    hasDetails: true
    onTriggered: activate()
    onDetailsRequested: activate()

    function position() { return Plasmoid.popupPosition(card, width / 2, height / 2); }
    function backgroundIndex() {
        let index = Plasmoid.systemTrayModel.index(entry.row, 0);
        while (index.valid && typeof index.model.mapToSource === "function") index = index.model.mapToSource(index);
        return index;
    }
    function activate() {
        if (entry.kind === "Plasmoid") {
            if (entry.applet.fullRepresentation && !entry.applet.preferredRepresentation) {
                openApplet(entry.id);
            } else {
                entry.applet.Plasmoid.activated();
            }
        } else if (entry.kind === "StatusNotifier") {
            if (entry.menuOnly) Plasmoid.openContextMenu(entry.source, position(), card);
            else Plasmoid.activate(entry.source, position(), card);
        } else if (entry.kind === "BackgroundApp") {
            const index = backgroundIndex();
            index.model.openBackgroundApp(index);
        }
    }
    function contextMenu() {
        if (entry.kind === "Plasmoid") Plasmoid.showPlasmoidMenu(entry.applet, 0, height);
        else if (entry.kind === "StatusNotifier") Plasmoid.openContextMenu(entry.source, position(), card);
        else backgroundMenu.open(position());
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: card.contextMenu()
    }
    TapHandler {
        acceptedButtons: Qt.MiddleButton
        onTapped: if (card.entry.kind === "StatusNotifier") Plasmoid.secondaryActivate(card.entry.source, card.position())
    }
    Keys.onMenuPressed: contextMenu()
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: wheel => {
            if (card.entry.kind === "StatusNotifier") {
                if (wheel.angleDelta.y) Plasmoid.scroll(card.entry.source, wheel.angleDelta.y, "vertical");
                if (wheel.angleDelta.x) Plasmoid.scroll(card.entry.source, wheel.angleDelta.x, "horizontal");
            }
            wheel.accepted = false;
        }
    }
    PlasmaExtras.Menu {
        id: backgroundMenu
        PlasmaExtras.MenuItem {
            text: qsTr("Quit %1").arg(card.title)
            icon: "application-exit"
            onClicked: {
                const index = card.backgroundIndex();
                index.model.stopBackgroundApp(index);
            }
        }
    }
}
