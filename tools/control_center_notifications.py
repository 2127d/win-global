# SPDX-License-Identifier: MIT
"""Polish native notification views while retaining KDE's models and actions."""
import re
import shutil


def style_notifications(destination, source, replace):
    directory = destination / 'notifications'
    # The native singleton constructs toasts from its compiled resource URL.
    # Use the same extracted singleton locally so both history and live banners
    # resolve the styled components, with one shared model/timeout/DND owner.
    (directory / 'global/qmldir').write_text('singleton Globals 1.0 Globals.qml\n')
    for path in directory.rglob('*.qml'):
        text = path.read_text()
        if re.search(r'Notifications(?:Applet)?\.Globals', text):
            relative = 'global' if path.parent == directory else '../global'
            text = replace(text, 'import QtQuick\n', f'import QtQuick\nimport "{relative}" as NotificationState\n')
            text = re.sub(r'Notifications(?:Applet)?\.Globals', 'NotificationState.Globals', text)
            path.write_text(text)

    def edit(name, changes):
        path = directory / name
        text = path.read_text()
        for old, new in changes:
            text = replace(text, old, new)
        path.write_text(text)

    edit('global/Globals.qml', [
        ('Kirigami.Units.gridUnit * 22 : Kirigami.Units.gridUnit * 18', 'Kirigami.Units.gridUnit * 22 : Kirigami.Units.gridUnit * 20'),
        ('property int popupEdgeDistance: Kirigami.Units.gridUnit * 2', 'property int popupEdgeDistance: 24'),
        ('property int popupSpacing: (popupLocation & Qt.AlignHCenter) && !Kirigami.Settings.isMobile ? Kirigami.Units.smallSpacing : Kirigami.Units.gridUnit', 'property int popupSpacing: 10'),
    ])
    edit('DraggableDelegate.qml', [
        ('    topPadding: 8\n    bottomPadding: 8', '    property bool inPopup: false\n    topPadding: inPopup ? 4 : 12\n    bottomPadding: inPopup ? 4 : 12'),
        ('        objectName: "sequoiaNotificationCard"', '        objectName: "sequoiaNotificationCard"\n        visible: !swipeDelegate.inPopup'),
    ])
    edit('NotificationPopup.qml', [
        ('    id: notificationPopup', '    id: notificationPopup\n    objectName: "sequoiaNotificationPopup"'),
        ('implicitHeight: notificationItem.implicitHeight', 'implicitHeight: notificationItem.implicitHeight + 8'),
        ('        DraggableDelegate {', '        DraggableDelegate {\n            inPopup: true'),
        ('topMargin: notificationPopup.modelInterface.closable || notificationPopup.modelInterface.dismissable || notificationPopup.modelInterface.configurable ? -notificationPopup.topPadding : 0', 'topMargin: 0'),
        ('            leftPadding: 0\n            rightPadding: 0', '            leftPadding: 4\n            rightPadding: 4'),
    ])
    edit('delegates/DelegatePopup.qml', [
        ('import QtQuick\n', 'import QtQuick\nimport QtQuick.Controls as Controls\n'),
        ('    PlasmaExtras.PlasmoidHeading {', '    Controls.Control {'),
        ('        topInset: 0', '        topInset: 0\n        topPadding: 0\n        background: null'),
        ('leftPadding: Layout.mirrored ? -delegateRoot.modelInterface.popupLeftPadding : 0', 'leftPadding: 0'),
        ('rightPadding: Layout.mirrored ? 0 : -delegateRoot.modelInterface.popupRightPadding', 'rightPadding: 0'),
        ('Layout.topMargin: delegateRoot.hasBodyText || icon.visible ? Kirigami.Units.smallSpacing : 0', 'Layout.topMargin: 8'),
        ('Layout.topMargin: summary.visible ? 0 : Kirigami.Units.smallSpacing', 'Layout.topMargin: summary.visible ? 5 : 8'),
        ('        implicitWidth: 4', '        implicitWidth: 3\n        radius: 1.5'),
    ])
    edit('components/NotificationHeader.qml', [
        ('spacing: Kirigami.Units.smallSpacing', 'spacing: 8'),
        ('Layout.preferredWidth: Kirigami.Units.iconSizes.small', 'Layout.preferredWidth: 24'),
        ('Layout.preferredHeight: Kirigami.Units.iconSizes.small', 'Layout.preferredHeight: 24'),
        ('        opacity: 0.9', '        opacity: 1\n        font.family: Kirigami.Theme.smallFont.family\n        font.pointSize: Kirigami.Theme.smallFont.pointSize\n        font.weight: Font.Medium\n        color: Kirigami.Theme.disabledTextColor'),
    ])
    edit('components/Summary.qml', [
        ('else if (jobState === NotificationManager.Notifications.JobStateStopped)', 'else if (modelInterface.jobState === NotificationManager.Notifications.JobStateStopped)'),
        ('    level: 4', '    level: 4\n    font.family: Kirigami.Theme.defaultFont.family\n    font.pointSize: Kirigami.Theme.defaultFont.pointSize\n    font.weight: Font.DemiBold\n    renderType: Text.NativeRendering'),
    ])
    edit('components/Body.qml', [
        ('    id: bodyLabel', '    id: bodyLabel\n    font: Kirigami.Theme.defaultFont\n    renderType: Text.NativeRendering'),
    ])
    edit('components/HeadingButtons.qml', [
        ('        opacity: 0.9', '        opacity: 1\n        font: Kirigami.Theme.smallFont\n        color: Kirigami.Theme.disabledTextColor'),
        ('icon.name: "window-close"', 'icon.name: "window-close-symbolic"'),
    ])
    path = directory / 'components/HeadingButtons.qml'
    text = path.read_text().replace('PlasmaComponents3.ToolButton {', 'NotificationToolButton {')
    path.write_text(text)
    edit('components/ActionContainer.qml', [
        ('            PlasmaComponents3.ToolButton {', '            NotificationActionButton {'),
    ])
    edit('components/FooterLoader.qml', [
        ('    visible: active && sourceComponent !== null', '    visible: active && sourceComponent !== null\n    Layout.topMargin: visible ? 10 : 0'),
    ])
    edit('delegates/DelegateHistory.qml', [
        ('        id: summary', '        id: summary\n        Layout.topMargin: 8'),
        ('        id: bodyLabel', '        id: bodyLabel\n        Layout.topMargin: summary.visible ? 5 : 8'),
    ])
    for path in (source / 'notifications').glob('*.qml'):
        shutil.copy2(path, directory / 'components' / path.name)
