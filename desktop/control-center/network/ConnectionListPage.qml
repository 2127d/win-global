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
    readonly property real naturalHeight: (categories.visible ? categories.implicitHeight + spacing : 0) + spacing
        + (connectivityMessage.visible ? connectivityMessage.implicitHeight + spacing : 0)
        + Math.max(120, connectionView.contentHeight) + connectionView.bottomMargin
    objectName:"networkConnectionList"

    required property PlasmaNM.NetworkStatus nmStatus
    property alias model: typeFilter.sourceModel
    property alias count: connectionView.count

    property string layoutMode: "spotlight"
    readonly property bool groupedList:layoutMode==="desk" || layoutMode==="compact"
    property var summaryModel: model
    Binding {
        target: toolbar
        property: "searchHost"
        when: connectionListPage.layoutMode !== "details"
        value: connectionView.headerItem?.searchHost ?? null
    }
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
        if(layoutMode==="spotlight" || layoutMode==="desk") {
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
            if(connectionListPage.layoutMode === "desk") {
                // Omit only connections already represented by the cards above.
                const connected=records.entries.filter(entry=>entry.active || entry.connecting);
                const cardNames=[connected.find(entry=>entry.type===PlasmaNM.Enums.Wireless)?.name,
                                connected.find(entry=>entry.type===PlasmaNM.Enums.Wired)?.name,
                                ...records.entries.filter(entry=>entry.type===PlasmaNM.Enums.Vpn).map(entry=>entry.name)];
                return !cardNames.includes(sourceModel.data(sourceModel.index(row, 0, parent), PlasmaNM.NetworkModel.ItemUniqueNameRole));
            }
            if(connectionListPage.layoutMode !== "compact" || connectionListPage.category === "all") return true;
            return sourceModel.data(sourceModel.index(row,0,parent),filterRole) === connectionListPage.categoryType;
        }
    }
    NetworkRecords { id:records; connectionModel:connectionListPage.summaryModel; onEntriesChanged:Qt.callLater(()=>typeFilter.invalidateFilter()) }
    RowLayout {
        id: categories
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
        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
        background:null

        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth - contentItem.leftMargin - contentItem.rightMargin
        PlasmaComponents3.ScrollBar.vertical: NetworkScrollBar {
            parent: scrollView
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom; margins: 4 }
            z: 2
        }
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

        layer.enabled: true
        layer.effect: GraphicsInfo.api === GraphicsInfo.Software ? null : edgeFade
        Component { id: edgeFade; MultiEffect { maskEnabled:true;maskSource:edgeMask;maskThresholdMin:0.5;maskSpreadAtMin:1;autoPaddingEnabled:false } }
        contentItem: ListView {
            id: connectionView
            clip:true

            footer: Item {
                width: connectionView.width
                height: connectionListPage.groupedList ? (emptyLabel.visible ? emptyLabel.implicitHeight : 0) + 12 : 0
                clip: true
                PlasmaComponents3.Label {
                    id: emptyLabel
                    visible: connectionListPage.groupedList && connectionView.count === 0
                    width: parent.width
                    leftPadding: 16; rightPadding: 16; topPadding: 10; bottomPadding: 12
                    wrapMode: Text.Wrap
                    color: Kirigami.Theme.disabledTextColor
                    text: toolbar.searchTextField.text ? i18n("No matches")
                        : connectionListPage.category === "vpn" ? i18n("No VPN connections")
                        : connectionListPage.category === "wifi" ? i18n("No Wi-Fi connections") : i18n("No available networks")
                }
            }
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

            header: Column {
                width: connectionView.width
                readonly property Item searchHost: connectionListPage.groupedList ? groupedSearch : summary.searchHost
                readonly property real summaryHeight: summary.height
                NetworkSummary {
                    id: summary
                    width: connectionView.width - connectionView.leftMargin - connectionView.rightMargin
                    layoutMode:connectionListPage.layoutMode
                    entries:records.entries
                    searchText:toolbar.searchTextField.text
                    searchHeight:toolbar.searchTextField.implicitHeight
                    internetAccess:connectionListPage.nmStatus.connectivity===NMQt.NetworkManager.Full
                    portal:connectionListPage.nmStatus.connectivity===NMQt.NetworkManager.Portal
                    emptyMessage:toolbar.searchTextField.text?i18n("No matches"):toolbar.displayplaneModeMessage?i18n("Wi-Fi is off in airplane mode"):toolbar.displayWifiOffMessage?i18n("Turn on Wi-Fi to see nearby networks"):i18n("No available networks")
                    onConfigureRequested:toolbar.settings()
                    onRefreshRequested:toolbar.scan()
                    onVpnToggleRequested:row=>{
                        const record=records.entries.find(entry=>entry.row===row);
                        if(!record || record.type!==PlasmaNM.Enums.Vpn || record.connecting) return;
                        if(record.active) handler.deactivateConnection(record.connectionPath,record.devicePath);
                        else handler.activateConnection(record.connectionPath,record.devicePath,record.specificPath);
                    }
                    onConnectionRequested: row => connectionListPage.showConnection(row)
                }
                Item {
                    visible: connectionListPage.groupedList
                    width: parent.width
                    height: toolbar.searchTextField.implicitHeight + 16
                    Item { id: groupedSearch; x: 12; y: 8; width: parent.width - 24; height: toolbar.searchTextField.implicitHeight }
                }
            }
            // No topMargin because ListSectionHeader and InlineMessage both bring
            // their own.
            bottomMargin: Kirigami.Units.smallSpacing * 2
            leftMargin: 0
            rightMargin: 0
            spacing: connectionListPage.groupedList?3:8
            model: typeFilter
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds
            section.property: connectionListPage.groupedList ? "Section" : ""
            section.delegate:Column {
                id:sectionHeader
                required property string section
                width: connectionView.width
                PlasmaComponents3.Label {
                    width:parent.width;leftPadding:16;rightPadding:16;topPadding:14;bottomPadding:6
                    text:sectionHeader.section;font.weight:Font.DemiBold
                    font.pointSize:Kirigami.Theme.defaultFont.pointSize*0.9
                    color:Kirigami.Theme.disabledTextColor
                }
            }
            highlight: null // The rounded delegate owns its highlight.
            highlightMoveDuration: Kirigami.Units.shortDuration
            highlightResizeDuration: Kirigami.Units.shortDuration
            delegate: ConnectionItem {
                listView: connectionView
                cardSurface.color: cardSurface.stateColor
                grouped:connectionListPage.groupedList
                width: connectionView.width-(grouped?24:0)
            }

            // Placeholder message
            Loader {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - Kirigami.Units.largeSpacing * 4)
                active: connectionView.count === 0 && connectionListPage.layoutMode === "details"
                visible: status === Loader.Ready && !!connectionListPage.model
                sourceComponent: PlasmaExtras.PlaceholderMessage {
                    iconName: "network-disconnect-symbolic"
                    text: i18n("This connection is no longer available.")
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
    Details.DetailSurface {
        objectName:"networkConnectionsIsland"
        parent:connectionView.contentItem
        visible:connectionListPage.groupedList
        z:-1;radius:24
        x:0;y:(connectionView.headerItem?.y ?? 0) + (connectionView.headerItem?.summaryHeight ?? 0)
        width:connectionView.width
        height:Math.max(0, connectionView.contentHeight - (connectionView.headerItem?.summaryHeight ?? 0))
    }
    Rectangle {
        id:edgeMask; width:scrollView.width;height:scrollView.height;visible:false;layer.enabled:true
        readonly property real fade: Math.min(0.12,16/Math.max(1,height))
        gradient:Gradient { GradientStop{position:0;color:connectionView.contentY>0?"transparent":"white"} GradientStop{position:edgeMask.fade;color:"white"} GradientStop{position:1-edgeMask.fade;color:"white"} GradientStop{position:1;color:connectionView.atYEnd?"white":"transparent"} }
    }
}
