// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.plasmoid

// One policy for both purpose-built controls and the remaining tray cards.
QtObject {
    id: items
    required property var configuration
    required property var sourceModel
    property var entries: []
    readonly property var customPlugins: [
        "org.kde.plasma.networkmanagement", "org.kde.plasma.bluetooth",
        "org.kde.plasma.notifications", "org.kde.plasma.volume",
        "org.kde.plasma.brightness", "org.kde.plasma.mediacontroller"
    ]
    readonly property var customItems: entries.filter(entry => customPlugins.includes(entry.id)).map(entry => entry.id)
    readonly property var otherItems: entries.filter(entry => !customPlugins.includes(entry.id))
    // Reading every setting here makes changes apply without restarting Plasma.
    readonly property var policy: ({
        all: configuration.showAllItems,
        shown: configuration.shownItems,
        popup: configuration.hiddenItems,
        enabled: configuration.extraItems,
        disabled: configuration.disabledStatusNotifiers
    })
    onPolicyChanged: Qt.callLater(rebuild)
    onSourceModelChanged: Qt.callLater(rebuild)
    Component.onCompleted: Qt.callLater(rebuild)

    function rebuild() {
        if (!sourceModel || !configuration) return;
        const result = [];
        const seen = new Set();
        for (let row = 0; row < sourceModel.rowCount(); ++row) {
            const index = sourceModel.index(row, 0);
            const value = name => {
                const role = sourceModel.KItemModels.KRoleNames.role(name);
                return role < 0 ? undefined : sourceModel.data(index, role);
            };
            const kind = value("itemType");
            const applet = kind === "Plasmoid" ? value("applet") : null;
            const id = kind === "Plasmoid" ? applet?.Plasmoid.pluginName
                : kind === "StatusNotifier" ? value("Id") : value("itemId");
            const identity = kind === "StatusNotifier" ? value("DataEngineSource") || id : id;
            if (!id || seen.has(identity) || policy.all || policy.shown.includes(id)) continue;
            if (kind === "Plasmoid" ? !policy.enabled.includes(id) : policy.disabled.includes(id)) continue;
            seen.add(identity);
            result.push({ id, kind, applet, row,
                title: kind === "StatusNotifier" ? value("Title") || value("ToolTipTitle") : value("name"),
                subtitle: kind === "StatusNotifier" ? value("ToolTipSubTitle") : value("message"),
                icon: kind === "StatusNotifier" ? value("Icon") || value("IconName") : value("icon"),
                source: value("DataEngineSource"), menuOnly: value("ItemIsMenu") });
        }
        entries = result;
    }
    readonly property Connections modelConnections: Connections {
        target: items.sourceModel
        function onDataChanged() { Qt.callLater(items.rebuild); }
        function onRowsInserted() { Qt.callLater(items.rebuild); }
        function onRowsRemoved() { Qt.callLater(items.rebuild); }
        function onRowsMoved() { Qt.callLater(items.rebuild); }
        function onModelReset() { Qt.callLater(items.rebuild); }
        function onLayoutChanged() { Qt.callLater(items.rebuild); }
    }
}
