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
    path.write_text(text)
    path = directory / 'main.xml'
    text = replace(path.read_text(), '<group name="General">', '<group name="General"><entry name="networkLayout" type="String"><default>spotlight</default></entry>')
    path.write_text(text)

    path = directory / 'ConnectionItem.qml'
    text = replace(path.read_text(), 'appletProxyModel.setData(appletProxyModel.index(index, 0), delay, PlasmaNM.NetworkModel.DelayModelUpdatesRole);',
                   'listView.model.setData(listView.model.index(index, 0), delay, PlasmaNM.NetworkModel.DelayModelUpdatesRole);')
    path.write_text(text)
