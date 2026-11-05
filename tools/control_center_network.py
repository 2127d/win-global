# SPDX-License-Identifier: MIT
"""Three network layouts retaining Plasma's native connection and QR flows."""
import shutil


def style_network(destination, source, replace):
    directory = destination / 'networkmanagement'
    for path in (source / 'network').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: mainWindow\n', '    id: mainWindow\n    Plasmoid.title: i18n("Networks")\n    readonly property bool sequoiaFooterActions: true\n')
    text = replace(text, 'Layout.minimumHeight: Kirigami.Units.gridUnit * 20', 'Layout.minimumHeight: 160')
    path.write_text(text)
    path = directory / 'main.xml'
    text = replace(path.read_text(), '<group name="General">', '<group name="General"><entry name="networkLayout" type="String"><default>spotlight</default></entry>')
    path.write_text(text)

    path = directory / 'ConnectionItem.qml'
    text = replace(path.read_text(), 'appletProxyModel.setData(appletProxyModel.index(index, 0), delay, PlasmaNM.NetworkModel.DelayModelUpdatesRole);',
                   'listView.model.setData(listView.model.index(index, 0), delay, PlasmaNM.NetworkModel.DelayModelUpdatesRole);')
    # The actions-to-content divider belongs to KDE's expandable delegate.
    # Hide it only while the Speed/Details content occupies its loader.
    text = replace(text, '            readonly property T.TabBar detailsTabBar: detailsTabBar', '''            id: networkDetails
            Binding {
                target: networkDetails.parent?.parent?.children.find(child =>
                    child.imagePath === "widgets/line" && child.elementId === "horizontal-line") ?? null
                property: "visible"
                value: false
                when: target !== null
                restoreMode: Binding.RestoreBindingOrValue
            }
            readonly property T.TabBar detailsTabBar: detailsTabBar''')
    # Reserve space in the expanded card for the graph's bottom legend.
    text = replace(text, 'height: trafficMonitorGraph.implicitHeight', '''readonly property real chartInset: Kirigami.Units.largeSpacing
                height: trafficMonitorGraph.implicitHeight + chartInset''')
    text = replace(text, '''id: trafficMonitorGraph
                    width: parent.width''', '''id: trafficMonitorGraph
                    x: parent.chartInset
                    width: Math.max(0, parent.width - 2 * parent.chartInset)''')
    # NetworkManager may temporarily withdraw counters during a model reset.
    text = replace(text, '''            connectionItem.rxSpeed = prevRxBytes === 0 ? 0 : (connectionItem.model.RxBytes - prevRxBytes) * 1000 / interval
            connectionItem.txSpeed = prevTxBytes === 0 ? 0 : (connectionItem.model.TxBytes - prevTxBytes) * 1000 / interval
            prevRxBytes = connectionItem.model.RxBytes
            prevTxBytes = connectionItem.model.TxBytes''', '''            const rx = connectionItem.model?.RxBytes ?? 0;
            const tx = connectionItem.model?.TxBytes ?? 0;
            connectionItem.rxSpeed = prevRxBytes > 0 ? Math.max(0, rx - prevRxBytes) * 1000 / interval : 0;
            connectionItem.txSpeed = prevTxBytes > 0 ? Math.max(0, tx - prevTxBytes) * 1000 / interval : 0;
            prevRxBytes = rx;
            prevTxBytes = tx;''')
    path.write_text(text)
