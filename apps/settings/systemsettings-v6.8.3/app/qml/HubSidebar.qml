// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQml.Models
import org.sequoia.components 1.0 as S
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

HubPanel {
    id: root
    objectName: "settingsNavigationSidebar"
    readonly property real headerHeight: 60
    property S.Theme iconTheme: AppearanceLibraryTheme {}
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
            Layout.preferredHeight: S.Tokens.controlHeight
            Kirigami.Icon {
                objectName: "settingsHeaderIcon"
                source: "preferences-system"
                Layout.preferredWidth: 28; Layout.preferredHeight: 28
                Layout.alignment: Qt.AlignVCenter
                isMask: false
                Accessible.ignored: true
            }
            QQC2.Label { text: i18n("System Settings"); font.weight: Font.DemiBold; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.05; Layout.fillWidth: true }

        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Kirigami.SearchField {
                id: searchField
                objectName: "settingsSearch"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.minimumHeight: S.Tokens.controlHeight
                Layout.maximumHeight: S.Tokens.controlHeight
                implicitHeight: S.Tokens.controlHeight
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
            S.CircleButton {
                id: showMenuButton
                objectName: "settingsShowMenu"
                property alias menu: sidebarMenu
                Layout.alignment: Qt.AlignVCenter
                Layout.minimumWidth: searchField.height
                Layout.maximumWidth: searchField.height
                Layout.minimumHeight: searchField.height
                Layout.maximumHeight: searchField.height
                theme: root.iconTheme
                label: i18n("Show menu")
                icon.name: "application-menu"
                down: pressed || sidebarMenu.visible
                QQC2.ToolTip.visible: (hovered || visualFocus) && !sidebarMenu.visible
                onClicked: sidebarMenu.visible ? sidebarMenu.close() : sidebarMenu.open()
                Accessible.role: Accessible.ButtonMenu
                AppearancePopupMenu {
                    id: sidebarMenu
                    objectName: "settingsShowMenuPopup"
                    trigger: showMenuButton
                    backdropItem: root
                    Instantiator {
                        model: ["highlight_changes", "show_irrelevant_modules", "report_bug_in_current_module",
                                "help_report_bug", "help_contents", "help_about_app", "help_about_kde"]
                        delegate: AppearancePillMenuItem {
                            required property string modelData
                            property var nativeAction: systemsettings.action(modelData)
                            objectName: "settingsAction-" + modelData
                            visible: !!nativeAction && nativeAction.visible
                            height: visible ? implicitHeight : 0
                            enabled: !!nativeAction && nativeAction.enabled
                            text: nativeAction ? nativeAction.text.replace(/&&/g, "\u0000").replace(/&/g, "").replace(/\u0000/g, "&") : ""
                            checkable: !!nativeAction && nativeAction.checkable
                            checked: !!nativeAction && nativeAction.checked
                            onTriggered: if (nativeAction) nativeAction.trigger()
                        }
                        onObjectAdded: function(index, object) { sidebarMenu.insertItem(index, object); }
                        onObjectRemoved: function(index, object) { sidebarMenu.removeItem(object); }
                    }
                }
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
        S.NavigationItem {
            objectName: "settingsHome"
            theme: root.iconTheme
            Layout.fillWidth: true
            Layout.minimumHeight: searchField.height
            Layout.maximumHeight: searchField.height
            text: i18n("Home")
            icon.name: "go-home"
            highlighted: navigation.home && !navigation.moduleOpen
            onClicked: navigation.browse("home")
        }
        S.NavigationItem {
            objectName: "settingsQuick"
            theme: root.iconTheme
            Layout.fillWidth: true
            Layout.minimumHeight: searchField.height
            Layout.maximumHeight: searchField.height
            visible: navigation.catalog.some(entry => entry.id === "kcm_landingpage")
            text: i18n("Quick Settings")
            icon.name: "preferences-desktop"
            highlighted: navigation.moduleOpen && navigation.currentModule === "kcm_landingpage"
            description: i18n("Light and dark themes and common settings")
            onClicked: navigation.open("kcm_landingpage")
        }
        Item { Layout.fillWidth: true; implicitHeight: 1 }
        QQC2.ScrollView {
            id: categoryScroll
            objectName: "settingsCategoryScroll"
            property ScrollEdgeFade edgeFade: ScrollEdgeFade {
                objectName: "settingsCategoryFade"
                viewport: categoryScroll.contentItem
            }
            padding: 0
            background: null
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            ColumnLayout {
                width: categoryScroll.availableWidth
                spacing: 3
                Repeater {
                    model: navigation.groups
                    S.NavigationItem {
                        required property var modelData
                        theme: root.iconTheme
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.minimumHeight: searchField.height
                        Layout.maximumHeight: searchField.height
                        text: modelData.name
                        icon.source: Qt.resolvedUrl(modelData.id + ".svg")
                        highlighted: navigation.selectedGroup === modelData.id && navigation.query.length === 0
                            && !(navigation.moduleOpen && navigation.currentModule === "kcm_landingpage")
                        description: modelData.description
                        onClicked: navigation.browse(modelData.id)
                    }
                }
            }
        }
        Item { Layout.fillWidth: true; implicitHeight: 1 }
        AppearanceActionPill {
            objectName: "settingsAll"
            id: settingsAll
            Layout.alignment: Qt.AlignHCenter
            text: i18n("All Settings")
            icon.name: "view-grid"
            highlighted: navigation.selectedGroup === "all" && navigation.query.length === 0
            onClicked: navigation.browse("all")
        }
    }
}
