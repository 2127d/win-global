// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    id: root
    objectName: "settingsNavigationSidebar"
    readonly property real headerHeight: 60
    implicitWidth: Math.max(236, Kirigami.Units.gridUnit * 13)
    implicitHeight: 640
    color: Kirigami.Theme.backgroundColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Shortcut { sequence: "Ctrl+F"; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "Alt+Left"; onActivated: navigation.back() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Kirigami.Icon { source: "preferences-system"; implicitWidth: 24; implicitHeight: 24 }
            QQC2.Label { text: i18n("Settings"); font.bold: true; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2; Layout.fillWidth: true }
            HamburgerMenuButton { }
        }
        Kirigami.SearchField {
            id: searchField
            objectName: "settingsSearch"
            Layout.fillWidth: true
            placeholderText: i18n("Search settings…")
            Accessible.name: placeholderText
            onTextChanged: {
                if (text !== navigation.query && !navigation.search(text)) text = navigation.query;
            }
            Connections {
                target: navigation
                function onChanged() { if (searchField.text !== navigation.query) searchField.text = navigation.query; }
            }
        }
        QQC2.Button {
            objectName: "settingsBack"
            Layout.fillWidth: true
            visible: navigation.moduleOpen || !navigation.home
            text: navigation.moduleOpen ? i18n("Back to overview") : i18n("Back to home")
            icon.name: root.LayoutMirroring.enabled ? "go-next" : "go-previous"
            onClicked: navigation.back()
        }
        QQC2.ItemDelegate {
            objectName: "settingsHome"
            Layout.fillWidth: true
            text: i18n("Home")
            icon.name: "go-home"
            highlighted: navigation.home && !navigation.moduleOpen
            onClicked: navigation.browse("home")
        }
        QQC2.ItemDelegate {
            objectName: "settingsQuick"
            Layout.fillWidth: true
            visible: navigation.catalog.some(entry => entry.id === "kcm_landingpage")
            text: i18n("Quick Settings")
            icon.name: "preferences-desktop"
            highlighted: navigation.moduleOpen && navigation.currentModule === "kcm_landingpage"
            onClicked: navigation.open("kcm_landingpage")
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.text: i18n("Light and dark themes and common settings")
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        Item { Layout.fillWidth: true; implicitHeight: 1 }
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            ColumnLayout {
                width: parent.width
                spacing: 3
                Repeater {
                    model: navigation.groups
                    QQC2.ItemDelegate {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: Math.max(42, implicitContentHeight + 16)
                        text: modelData.name
                        icon.name: modelData.icon
                        highlighted: navigation.selectedGroup === modelData.id && navigation.query.length === 0
                            && !(navigation.moduleOpen && navigation.currentModule === "kcm_landingpage")
                        onClicked: navigation.browse(modelData.id)
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: modelData.description
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }
        Item { Layout.fillWidth: true; implicitHeight: 1 }
        QQC2.ItemDelegate {
            objectName: "settingsAll"
            Layout.fillWidth: true
            text: i18n("All Settings")
            icon.name: "view-list-details"
            highlighted: navigation.selectedGroup === "all" && navigation.query.length === 0
            onClicked: navigation.browse("all")
        }
    }
}
