/*
    SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import "../details" as Details
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.plasma.plasmoid

Details.DetailRepresentation {
    id: full

    required property PlasmaNM.Handler nmHandler
    required property PlasmaNM.NetworkStatus nmStatus

    readonly property alias stackDepth: stack.depth

    function popStack(): void {
        stack.pop()
    }

    readonly property string layoutMode: ["spotlight","desk","compact"].includes(Plasmoid.configuration.networkLayout) ? Plasmoid.configuration.networkLayout : "spotlight"
    function setLayout(mode) { if (["spotlight","desk","compact"].includes(mode)) Plasmoid.configuration.networkLayout=mode; }
    collapseMarginsHint: true
    padding: 0
    leftPadding:0;rightPadding:0;topPadding:0;bottomPadding:0
    background: null
    implicitWidth: 418
    implicitHeight: 600

    Component {
        id: networkModelComponent
        PlasmaNM.NetworkModel {}
    }

    property PlasmaNM.NetworkModel connectionModel: null

    PlasmaNM.AppletProxyModel {
        id: appletProxyModel

        sourceModel: full.connectionModel
    }

    header: Toolbar {
        id: toolbar
        layoutMode:full.layoutMode
        hasConnections: connectionListPage.count > 0
        backOnly:stack.depth > 1
        onBackRequested:stack.pop()
        visible: stack.depth === 1 || Plasmoid.containment.pluginName !== "org.sequoia.systemtray"
    }
    footer: NetworkActions {
        visible: stack.depth === 1
        controller: toolbar
        layoutMode: full.layoutMode
        onLayoutSelected: mode => full.setLayout(mode)
    }

    Connections {
        target: full.nmHandler
        function onWifiCodeReceived(data, ssid, password) {
            if (data.length === 0) {
                console.error("Cannot create QR code component: Unsupported connection");
                return;
            }

            const showQRComponent = Qt.createComponent("ShareNetworkQrCodePage.qml");
            if (showQRComponent.status === Component.Error) {
                console.warn("Cannot create QR code component:", showQRComponent.errorString());
                return;
            }

            mainWindow.expanded = true; // just in case.
            stack.push(showQRComponent, {
                content: data,
                ssid,
                password,
            });
        }
    }

    Keys.forwardTo: [stack.currentItem]
    Keys.onPressed: event => {
        if (event.modifiers & Qt.ControlModifier && event.key == Qt.Key_F) {
            if(stack.depth>1)stack.pop(null);
            toolbar.revealSearch();
            event.accepted = true;
        } else if (event.key === Qt.Key_Back || (event.modifiers & Qt.AltModifier && event.key == Qt.Key_Left)) {
            if (stack.depth > 1) {
                stack.pop();
                event.accepted = true;
            }
        } else {
            event.accepted = false;
        }
    }

    QQC2.StackView {
        id: stack
        anchors.fill: parent
        initialItem: ConnectionListPage {
            id: connectionListPage
            model: mainWindow.expanded ? appletProxyModel : null
            nmStatus: full.nmStatus
            layoutMode: full.layoutMode
            onDetailRequested:name=>stack.push(connectionDetailsComponent,{detailName:name})
        }
    }

    Component {
        id:connectionDetailsComponent
        ConnectionListPage {
            model:appletProxyModel
            nmStatus:full.nmStatus
            layoutMode:"details"
        }
    }

    Connections {
        target: mainWindow
        function onExpandedChanged(expanded) {
            if (expanded) {
                handler.requestScan();
                if (!full.connectionModel) {
                    full.connectionModel = networkModelComponent.createObject(full);
                }
            } else {
                if (full.connectionModel) {
                    full.connectionModel.destroy();
                    full.connectionModel = null;
                }
            }
        }
    }
}
