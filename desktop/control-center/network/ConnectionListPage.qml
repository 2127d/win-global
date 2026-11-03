/*
    SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick
import org.kde.kitemmodels as KItemModels
import QtQuick.Effects
import "../details" as Details
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.networkmanager as NMQt

ColumnLayout {
    id: connectionListPage
    objectName:"networkConnectionList"

    required property PlasmaNM.NetworkStatus nmStatus
    property alias model: typeFilter.sourceModel
    property alias count: connectionView.count

    property string layoutMode: "spotlight"
    property string category: "all"
    property string detailName:""
    signal detailRequested(string name)
    onCountChanged:if(layoutMode==="details" && count>0) Qt.callLater(()=>showConnection(0))
    onDetailNameChanged:Qt.callLater(resetFilter)
    readonly property int categoryType: category === "wifi" ? PlasmaNM.Enums.Wireless : PlasmaNM.Enums.Vpn
    function resetFilter() {
        // Release native password-entry update locks before removing delegates.
        for(let i=0;i<connectionView.count;i++) {
            const item=connectionView.itemAtIndex(i);
            if(item?.expanded) item.collapse();
        }
        typeFilter.invalidateFilter();
    }
    onCategoryChanged:Qt.callLater(resetFilter)
    onLayoutModeChanged:Qt.callLater(resetFilter)
    function showConnection(row) {
        if(layoutMode==="spotlight") {
            const record=records.entries.find(entry=>entry.row===row);
            if(record) detailRequested(record.name);
            return;
        }
        connectionView.currentIndex=row;
        connectionView.positionViewAtIndex(row,ListView.Contain);
        Qt.callLater(() => { const item=connectionView.itemAtIndex(row); if (item) {item.expand();item.forceActiveFocus();} });
    }
    spacing: 8
    KItemModels.KSortFilterProxyModel {
        id:typeFilter
        filterRoleName:"Type"
        filterRowCallback: (row, parent) => {
            if(connectionListPage.layoutMode === "spotlight") return false;
            if(connectionListPage.layoutMode === "details") return sourceModel.data(sourceModel.index(row,0,parent),PlasmaNM.NetworkModel.ItemUniqueNameRole) === connectionListPage.detailName;
            if(connectionListPage.layoutMode !== "compact" || connectionListPage.category === "all") return true;
            return sourceModel.data(sourceModel.index(row,0,parent),filterRole) === connectionListPage.categoryType;
        }
    }
    NetworkRecords { id:records; connectionModel:connectionListPage.model }
    RowLayout {
        Layout.fillWidth:true; visible:connectionListPage.layoutMode === "compact"; spacing:6
        Repeater {
            model:[{id:"all",label:qsTr("All")},{id:"wifi",label:qsTr("Wi-Fi")},{id:"vpn",label:qsTr("VPN")}]
            delegate:NetworkActionButton {
                required property var modelData
                Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:1
                text:modelData.label;checkable:true;checked:connectionListPage.category===modelData.id
                onClicked:connectionListPage.category=modelData.id
            }
        }
    }

    Keys.forwardTo: [connectionView]

    Kirigami.InlineMessage {
        id: connectivityMessage
        Layout.fillWidth: true
        Layout.preferredHeight: contentItem.implicitHeight + topPadding + bottomPadding
        type: Kirigami.MessageType.Information
        position: Kirigami.InlineMessage.Position.Header
        icon.name: "dialog-password"
        text: i18n("You need to log in to this network")
        visible: connectionListPage.nmStatus.connectivity === NMQt.NetworkManager.Portal

        actions: Kirigami.Action {
            text: i18nc("@action:button", "Log in")
            onTriggered: {
                Qt.openUrlExternally(connectionListPage.nmStatus.networkCheckUrl);
            }
        }
    }

    PlasmaComponents3.ScrollView {
        id: scrollView
        padding:0
        background:null

        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin
        PlasmaComponents3.ScrollBar.vertical.policy: PlasmaComponents3.ScrollBar.AlwaysOff
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect { maskEnabled:true;maskSource:edgeMask;maskThresholdMin:0.5;maskSpreadAtMin:1;autoPaddingEnabled:false }
        contentItem: ListView {
            id: connectionView
            clip:true

            property int currentVisibleButtonIndex: -1

            Keys.onDownPressed: event => {
                connectionView.incrementCurrentIndex();
                connectionView.currentItem?.forceActiveFocus();
            }
            Keys.onUpPressed: event => {
                if (connectionView.currentIndex === 0) {
                    connectionView.currentIndex = -1;
                    toolbar.searchTextField.forceActiveFocus();
                    toolbar.searchTextField.selectAll();
                } else {
                    event.accepted = false;
                }
            }

            header: NetworkSummary {
                width: connectionView.width - connectionView.leftMargin - connectionView.rightMargin
                layoutMode:connectionListPage.layoutMode
                entries:records.entries
                internetAccess:connectionListPage.nmStatus.connectivity===NMQt.NetworkManager.Full
                portal:connectionListPage.nmStatus.connectivity===NMQt.NetworkManager.Portal
                emptyMessage:toolbar.searchTextField.text?i18n("No matches"):toolbar.displayplaneModeMessage?i18n("Wi-Fi is off in airplane mode"):toolbar.displayWifiOffMessage?i18n("Turn on Wi-Fi to see nearby networks"):i18n("No available networks")
                onConfigureRequested:toolbar.settings()
                onSearchRequested:toolbar.revealSearch()
                onRefreshRequested:toolbar.scan()
                onVpnToggleRequested:row=>{
                    const record=records.entries.find(entry=>entry.row===row);
                    if(!record || record.type!==PlasmaNM.Enums.Vpn || record.connecting) return;
                    if(record.active) handler.deactivateConnection(record.connectionPath,record.devicePath);
                    else handler.activateConnection(record.connectionPath,record.devicePath,record.specificPath);
                }
                onConnectionRequested: row => connectionListPage.showConnection(row)
            }
            // No topMargin because ListSectionHeader and InlineMessage both bring
            // their own.
            bottomMargin: Kirigami.Units.smallSpacing * 2
            leftMargin: 0
            rightMargin: 0
            spacing: 8
            model: typeFilter
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds
            section.property: connectionListPage.layoutMode === "compact" || connectionListPage.layoutMode === "details" ? "" : "Section"
            section.delegate: PlasmaExtras.ListSectionHeader {
                width: connectionView.width - connectionView.leftMargin - connectionView.rightMargin
                text: section
            }
            highlight: null // The rounded delegate owns its highlight.
            highlightMoveDuration: Kirigami.Units.shortDuration
            highlightResizeDuration: Kirigami.Units.shortDuration
            delegate: ConnectionItem {
                listView: connectionView
                width: connectionView.width
            }

            // Placeholder message
            Loader {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 4)
                active: connectionView.count === 0 && connectionListPage.layoutMode!=="spotlight"
                asynchronous: true
                visible: status === Loader.Ready && !!connectionListPage.model
                sourceComponent: PlasmaExtras.PlaceholderMessage {
                    iconName: {
                        if (toolbar.displayplaneModeMessage) {
                            return "network-flightmode-on"
                        }
                        if (toolbar.displayWifiOffMessage) {
                            return "network-wireless-off"
                        }
                        if (toolbar.displayWifiConnectingMessage) {
                            return "view-refresh-symbolic"
                        }
                        if (toolbar.displayWwanMessage) {
                            return "network-mobile-off"
                        }
                        return "edit-none"
                    }
                    text: {
                        if(connectionListPage.layoutMode==="details")return i18n("This connection is no longer available.");
                        if (toolbar.displayplaneModeMessage) {
                            return i18n("Airplane mode is enabled")
                        }
                        if (toolbar.displayWifiOffMessage) {
                            if (toolbar.displayWwanMessage) {
                                return i18n("Wi-Fi and mobile data are deactivated")
                            }
                            return i18n("Wi-Fi is deactivated")
                        }
                        if (toolbar.displayWifiConnectingMessage) {
                            return i18n("Looking for Wi-Fi networks")
                        }
                        if (toolbar.displayWwanMessage) {
                            return i18n("Mobile data is deactivated")
                        }
                        if (toolbar.searchTextField.text.length > 0) {
                            return i18n("No matches")
                        }
                        if (connectionListPage.layoutMode === "compact" && connectionListPage.category !== "all") return connectionListPage.category === "wifi" ? i18n("No Wi-Fi connections") : i18n("No VPN connections");
                        if (connectionListPage.nmStatus.connectivity === NMQt.NetworkManager.Full) {
                            return i18n("No available connections")
                        }
                        return nmStatus.checkUnknownReason()
                    }
                    explanation: {
                        if (toolbar.searchTextField.text.length === 0) {
                            return ""
                        }
                        if (toolbar.displayplaneModeMessage) {
                            return i18nc("@info:placeholder", "Turn off airplane mode to search for networks")
                        }
                        if (toolbar.displayWifiOffMessage) {
                            if (toolbar.displayWwanMessage) {
                                return i18nc("@info:placeholder", "Turn on Wi-Fi or mobile data to search for networks")
                            }
                            return i18nc("@info:placeholder", "Turn on Wi-Fi to search for networks")
                        }
                        if (toolbar.displayWwanMessage) {
                            return i18nc("@info:placeholder", "Turn on mobile data to search for networks")
                        }
                    }
                }
            }

            function newIndexSelected(index) {
                for (let i = 0; i < connectionView.count; i++) {
                    if (i === index)
                        continue

                    let item = connectionView.itemAtIndex(i)
                    if (item && item.expanded && item.customExpandedViewContent === item.passwordDialogComponent) {
                        item.collapse()
                    }
                }
            }
        }
    }
    Rectangle {
        id:edgeMask; width:scrollView.width;height:scrollView.height;visible:false;layer.enabled:true
        readonly property real fade: Math.min(0.12,16/Math.max(1,height))
        gradient:Gradient { GradientStop{position:0;color:connectionView.contentY>0?"transparent":"white"} GradientStop{position:edgeMask.fade;color:"white"} GradientStop{position:1-edgeMask.fade;color:"white"} GradientStop{position:1;color:connectionView.atYEnd?"white":"transparent"} }
    }
}
