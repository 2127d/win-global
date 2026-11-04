# SPDX-License-Identifier: MIT
"""Power presentation using native battery, profile, and inhibition controls."""
import shutil


def style_power(destination, source, replace):
    directory = destination / 'battery'
    for path in (source / 'power').glob('*.qml'):
        if path.name != 'Preview.qml':
            shutil.copy2(path, directory / path.name)
    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: batterymonitor\n',
                   '    id: batterymonitor\n    readonly property bool sequoiaFooterActions: true\n')
    path.write_text(text)
    # Keep KDE's per-application allow/unblock behavior, reasons and lid warning.
    # The redesigned view supplies the manual control and its explanatory text.
    path = directory / 'InhibitionItem.qml'
    text = replace(path.read_text(), '    id: root\n', '''    id: root
    property bool detailsOnly: false
    leftPadding: detailsOnly ? 0 : 12
    rightPadding: detailsOnly ? 0 : 12
    topPadding: detailsOnly ? 0 : 10
    bottomPadding: detailsOnly ? 0 : 10
''')
    text = replace(text, '    background.visible: true', '    background.visible: !detailsOnly')
    text = replace(text, '            id: inhibitionIcon\n', '            id: inhibitionIcon\n            visible: !root.detailsOnly\n')
    text = replace(text, '                id: inhibitionSwitch\n', '                id: inhibitionSwitch\n                visible: !root.detailsOnly\n')
    text = replace(text, '                visible: root.isManuallyInhibited\n', '                visible: root.isManuallyInhibited && !root.detailsOnly\n')
    path.write_text(text)
