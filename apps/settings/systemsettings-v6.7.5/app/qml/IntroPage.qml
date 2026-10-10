// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    id: root
    objectName: "settingsOverview"
    color: Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Math.max(20, Kirigami.Units.largeSpacing * 2)
        spacing: 8
        QQC2.Label {
            text: navigation.home ? i18n("SYSTEM SETTINGS") : i18n("YOUR COMPUTER")
            color: Kirigami.Theme.disabledTextColor
            font.bold: true
            font.pointSize: Kirigami.Theme.smallFont.pointSize
        }
        Kirigami.Heading {
            Layout.fillWidth: true
            text: navigation.title
            level: 1
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.8
            wrapMode: Text.WordWrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: navigation.description
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
        }
        Item { implicitHeight: 12 }
        QQC2.ScrollView {
            id: homeScroll
            visible: navigation.home
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            GridLayout {
                width: homeScroll.availableWidth
                columns: width >= Kirigami.Units.gridUnit * 30 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 12
                Repeater {
                    model: navigation.groups
                    QQC2.AbstractButton {
                        id: card
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        implicitHeight: Math.max(126, contentItem.implicitHeight + 32)
                        padding: 16
                        hoverEnabled: true
                        Accessible.name: modelData.name
                        Accessible.description: modelData.description
                        onClicked: navigation.browse(modelData.id)
                        background: Rectangle {
                            radius: 12
                            Kirigami.Theme.colorSet: Kirigami.Theme.View
                            Kirigami.Theme.inherit: false
                            color: card.down ? Kirigami.Theme.alternateBackgroundColor : Kirigami.Theme.backgroundColor
                            border.width: card.activeFocus || card.hovered ? 2 : 1
                            border.color: card.activeFocus || card.hovered ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.10)
                        }
                        contentItem: ColumnLayout {
                            spacing: 8
                            RowLayout {
                                Kirigami.Icon { source: card.modelData.icon; implicitWidth: 28; implicitHeight: 28 }
                                Item { Layout.fillWidth: true }
                                QQC2.Label { text: card.modelData.count; color: Kirigami.Theme.disabledTextColor; Accessible.ignored: true }
                                Kirigami.Icon { source: root.LayoutMirroring.enabled ? "go-previous" : "go-next"; implicitWidth: 16; implicitHeight: 16; opacity: 0.5 }
                            }
                            QQC2.Label { Layout.fillWidth: true; text: card.modelData.name; font.bold: true; wrapMode: Text.WordWrap }
                            QQC2.Label {
                                Layout.fillWidth: true
                                text: card.modelData.description
                                color: Kirigami.Theme.disabledTextColor
                                wrapMode: Text.WordWrap
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }
                }
            }
        }
        QQC2.ScrollView {
            id: resultsScroll
            visible: !navigation.home
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            ListView {
                id: results
                objectName: "settingsResults"
                clip: true
                model: navigation.entries
                spacing: 6
                keyNavigationEnabled: true
                activeFocusOnTab: true
                delegate: QQC2.ItemDelegate {
                    id: row
                    required property var modelData
                    required property int index
                    width: results.width
                    implicitHeight: Math.max(78, contentItem.implicitHeight + 24)
                    padding: 12
                    highlighted: ListView.isCurrentItem && results.activeFocus
                    onClicked: { results.currentIndex = index; navigation.open(modelData.id); }
                    contentItem: RowLayout {
                        spacing: 14
                        Kirigami.Icon { source: row.modelData.icon; implicitWidth: 32; implicitHeight: 32 }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            QQC2.Label { Layout.fillWidth: true; text: row.modelData.name; font.bold: true; wrapMode: Text.WordWrap }
                            QQC2.Label {
                                Layout.fillWidth: true
                                text: row.modelData.description || row.modelData.groupName
                                wrapMode: Text.WordWrap
                                color: Kirigami.Theme.disabledTextColor
                            }
                            QQC2.Label {
                                visible: navigation.query.length > 0 || navigation.selectedGroup === "all"
                                text: row.modelData.groupName
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                        Kirigami.Badge {
                            visible: row.modelData.changedFromDefault && systemsettings.defaultsIndicatorsVisible
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            type: Kirigami.Badge.Type.Warning
                        }
                        Kirigami.Icon { source: root.LayoutMirroring.enabled ? "go-previous" : "go-next"; implicitWidth: 20; implicitHeight: 20; opacity: 0.5 }
                    }
                    background: Rectangle {
                        radius: 10
                        Kirigami.Theme.colorSet: Kirigami.Theme.View
                        Kirigami.Theme.inherit: false
                        color: row.hovered || row.highlighted ? Kirigami.Theme.alternateBackgroundColor : Kirigami.Theme.backgroundColor
                        border.width: row.activeFocus ? 2 : 1
                        border.color: row.activeFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.08)
                    }
                    Accessible.name: modelData.name
                    Accessible.description: modelData.description
                }
                Keys.onReturnPressed: { if (currentIndex >= 0) navigation.open(model[currentIndex].id); }
                Keys.onEnterPressed: { if (currentIndex >= 0) navigation.open(model[currentIndex].id); }
                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    visible: results.count === 0
                    text: navigation.query.length > 0 ? i18n("No matching settings") : i18n("No settings in this category")
                    explanation: navigation.query.length > 0 ? i18n("Try a shorter term, or browse All Settings.") : i18n("Available settings appear here when installed.")
                    icon.name: "edit-find"
                }
            }
        }
    }
}
