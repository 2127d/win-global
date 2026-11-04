# SPDX-License-Identifier: MIT
"""Arrange the notification center without replacing KDE's notification delegates."""


def style_notification_history(destination, replace):
    directory = destination / 'notifications'
    path = directory / 'FullRepresentation.qml'
    text = path.read_text()
    text = replace(text, 'Details.DetailRepresentation {', '''PlasmaExtras.Representation {
    objectName: "sequoiaDetailPage"
    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
    background: null
    readonly property real sequoiaPreferredHeight: header.implicitHeight + actionIsland.implicitHeight + 24
        + (list.count > 0 ? list.contentHeight : (emptyHistory.item?.implicitHeight ?? 180))''')
    text = replace(text, 'Keys.onDownPressed: dndCheck.forceActiveFocus(Qt.TabFocusReason);', 'Keys.onDownPressed: quietCard.toggle.forceActiveFocus(Qt.TabFocusReason);')
    start = text.index('    header: Details.DetailHeading {')
    end = text.index('    PlasmaComponents3.ScrollView {', start)
    text = text[:start] + '''    header: PlasmaExtras.PlasmoidHeading {
        objectName: "notificationCenterHeading"
        background: Item {}
        leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
        leftPadding: 0; rightPadding: 0; topPadding: 6; bottomPadding: 10
        contentItem: ColumnLayout {
            spacing: 12
            Components.NotificationQuietCard {
                id: quietCard
                Layout.fillWidth: true
                notificationSettings: fullRepresentationRoot.notificationSettings
                inhibition: NotificationState.Globals
                serverAvailable: NotificationManager.Server.valid
                nextFocusItem: list
            }
            PlasmaComponents3.Label {
                Layout.fillWidth: true
                visible: fullRepresentationRoot.historyModel.count > 0
                text: i18n("Recent activity")
                font.weight: Font.DemiBold
                color: Kirigami.Theme.disabledTextColor
            }
        }
    }

    Details.DetailSurface {
        id: actionIsland
        objectName: "notificationActionIsland"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, footerActions.implicitWidth + 16)
        implicitHeight: footerActions.implicitHeight + 8
        radius: height / 2; elevated: true
        RowLayout {
            id: footerActions
            anchors.centerIn: parent; spacing: 2
            Components.NotificationFooterButton {
                id: settingsButton
                objectName: "notificationSettings"
                flat: true; text: i18n("Settings"); icon.name: "configure-symbolic"
                visible: Plasmoid.internalAction("configure")?.visible ?? false
                onClicked: Plasmoid.internalAction("configure").trigger()
            }
            Rectangle { visible: settingsButton.visible; width: 1; height: 18; color: Qt.alpha(Kirigami.Theme.textColor, 0.15) }
            Components.NotificationFooterButton {
                objectName: "notificationClearHistory"
                flat: true; text: i18n("Clear history"); icon.name: "edit-clear-history"
                enabled: fullRepresentationRoot.historyModel.expiredNotificationsCount > 0
                onClicked: fullRepresentationRoot.clearHistoryAction.trigger()
            }
        }
    }

''' + text[end:]
    text = replace(text, '        anchors.fill: parent\n        background: null', '''        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: actionIsland.top; anchors.bottomMargin: 12
        // Match the heading width even while the native scrollbar is visible.
        leftPadding: 0; rightPadding: 0
        PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff
        background: null''')
    text = replace(text, '            id: list\n', '            id: list\n            objectName: "notificationHistoryList"\n            contentWidth: width\n            clip: true\n            layer.enabled: true // Keep delegate glass effects inside the scrolling viewport.\n')
    text = replace(text, 'topMargin: Kirigami.Units.largeSpacing', 'topMargin: 6')
    text = replace(text, '\n            spacing: Kirigami.Units.smallSpacing', '\n            spacing: 10')
    text = replace(text, 'KeyNavigation.up: dndCheck', 'KeyNavigation.up: quietCard.toggle')
    text = replace(text, '''highlight: KSvg.FrameSvgItem {
                imagePath: "widgets/listitem"
                prefix: "pressed"
            }''', 'highlight: null // Each rounded card paints its own keyboard focus.')
    start = text.index('            section {')
    end = text.index('            delegate: DraggableDelegate {', start)
    # Keep sections for native group expansion, without a second separator surface.
    text = text[:start] + '''            section {
                property: "desktopEntry"
                criteria: ViewSection.FullString
            }

''' + text[end:]
    text = replace(text, '                id: delegate\n', '''                id: delegate
                objectName: "notificationHistoryCard:" + index
''')
    text = replace(text, '''                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.gridUnit * 4)''', '''                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                anchors.topMargin: 6''')
    text = replace(text, '''                PlasmaExtras.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width

                    iconName: "checkmark"
                    text: i18n("No unread notifications")
                }''', '                Components.NotificationEmptyState {}')
    text = replace(text, '                PlasmaExtras.PlaceholderMessage {', '                Components.NotificationEmptyState {')
    text = replace(text, '''                    anchors.centerIn: parent
                    width: parent.width

''', '')
    text = replace(text, 'text: i18n("Notification service not available")', 'title: i18n("Notification service not available")')
    text = replace(text, '            Loader {\n                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right', '            Loader {\n                id: emptyHistory\n                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right')
    path.write_text(text)

    path = directory / 'DraggableDelegate.qml'
    text = replace(path.read_text(), '        visible: !swipeDelegate.inPopup', '        visible: !swipeDelegate.inPopup\n        radius: 20')
    text = replace(text, '        focused: swipeDelegate.activeFocus', '        focused: swipeDelegate.activeFocus || (swipeDelegate.ListView.isCurrentItem && (swipeDelegate.ListView.view?.activeFocus ?? false))')
    path.write_text(text)

    path = directory / 'main.qml'
    text = replace(path.read_text(), '    id: root\n', '''    id: root
    Plasmoid.title: i18n("Notifications")
    readonly property bool sequoiaFooterActions: true
    readonly property bool sequoiaHistoryEmpty: historyModel.count === 0
''')
    text = replace(text, 'visible: historyModel.expiredNotificationsCount > 0', 'visible: !root.expanded && historyModel.expiredNotificationsCount > 0')
    path.write_text(text)

