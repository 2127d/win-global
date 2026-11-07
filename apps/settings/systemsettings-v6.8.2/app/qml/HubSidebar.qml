// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

HubPanel {
    id: root
    objectName: "settingsNavigationSidebar"
    readonly property real headerHeight: 60
    readonly property color sidebarText: Kirigami.Theme.textColor
    implicitWidth: Math.max(256, Kirigami.Units.gridUnit * 14)
    implicitHeight: 640
    tint: .86
    radius: 20
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
            HubIcon { symbol: "settings"; implicitWidth: 24; implicitHeight: 24 }
            QQC2.Label { text: i18n("System Settings"); font.weight: Font.DemiBold; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.05; Layout.fillWidth: true }
            HamburgerMenuButton { }
        }
        Kirigami.SearchField {
            id: searchField
            objectName: "settingsSearch"
            Layout.fillWidth: true
            implicitHeight: 42
            background: HubPanel { radius: 21; tint: .30; emphasized: searchField.activeFocus }
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
            flat: true
            text: navigation.moduleOpen ? i18n("Back to overview") : i18n("Back to home")
            icon.name: root.LayoutMirroring.enabled ? "go-next" : "go-previous"
            onClicked: navigation.back()
        }
        QQC2.ItemDelegate {
            objectName: "settingsHome"
            id: settingsHome
            implicitHeight: 43
            background: HubPanel { radius: 14; accent: settingsHome.highlighted; tint: settingsHome.hovered ? .36 : .22; emphasized: settingsHome.activeFocus; opacity: settingsHome.highlighted || settingsHome.hovered || "settingsHome" === "settingsAll" ? 1 : 0 }
            contentItem: RowLayout { spacing:10
                HubIcon { symbol:"home"; implicitWidth:23; implicitHeight:23 }
                QQC2.Label { text: settingsHome.text; color:root.sidebarText; Layout.fillWidth:true }
            }
            Layout.fillWidth: true
            text: i18n("Home")
            icon.name: "go-home"
            highlighted: navigation.home && !navigation.moduleOpen
            onClicked: navigation.browse("home")
        }
        QQC2.ItemDelegate {
            objectName: "settingsQuick"
            id: settingsQuick
            implicitHeight: 43
            background: HubPanel { radius: 14; accent: settingsQuick.highlighted; tint: settingsQuick.hovered ? .36 : .22; emphasized: settingsQuick.activeFocus; opacity: settingsQuick.highlighted || settingsQuick.hovered || "settingsQuick" === "settingsAll" ? 1 : 0 }
            contentItem: RowLayout { spacing:10
                HubIcon { symbol:"sliders"; implicitWidth:23; implicitHeight:23 }
                QQC2.Label { text: settingsQuick.text; color:root.sidebarText; Layout.fillWidth:true }
            }
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
                        id: categoryPill
                        contentItem: RowLayout {
                            spacing: 10
                            HubIcon { symbol: categoryPill.modelData.id; implicitWidth: 23; implicitHeight: 23 }
                            QQC2.Label { Layout.fillWidth: true; text: categoryPill.modelData.name; color: root.sidebarText; elide: Text.ElideRight }
                        }
                        required property var modelData
                        background: HubPanel { color: categoryPill.highlighted ? Qt.alpha(Kirigami.Theme.highlightColor, .20) : Qt.alpha(Kirigami.Theme.backgroundColor, .3); radius: 13; emphasized: categoryPill.activeFocus; tint: categoryPill.highlighted ? .7 : categoryPill.hovered ? .3 : 0; opacity: categoryPill.highlighted || categoryPill.hovered || categoryPill.activeFocus ? 1 : 0 }
                        Layout.fillWidth: true
                        implicitHeight: Math.max(43, implicitContentHeight + 16)
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
            id: settingsAll
            implicitHeight: 43
            background: HubPanel { radius: 14; accent: settingsAll.highlighted; tint: settingsAll.hovered ? .36 : .22; emphasized: settingsAll.activeFocus; opacity: settingsAll.highlighted || settingsAll.hovered || "settingsAll" === "settingsAll" ? 1 : 0 }
            contentItem: RowLayout { spacing:10
                HubIcon { symbol:"all"; implicitWidth:23; implicitHeight:23 }
                QQC2.Label { text: settingsAll.text; color:root.sidebarText; Layout.fillWidth:true }
            }
            Layout.fillWidth: true
            text: i18n("All Settings")
            icon.name: "view-list-details"
            highlighted: navigation.selectedGroup === "all" && navigation.query.length === 0
            onClicked: navigation.browse("all")
        }
    }
}
