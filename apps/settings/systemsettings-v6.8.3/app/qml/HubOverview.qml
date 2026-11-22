// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
Rectangle {
    id: root
    objectName: "settingsOverview"
    color: appearancePreview.canvasColor
    bottomRightRadius: root.LayoutMirroring.enabled ? 0 : 20
    bottomLeftRadius: root.LayoutMirroring.enabled ? 20 : 0
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    readonly property bool isCategory: navigation.query.length === 0 && navigation.selectedGroup !== "home" && navigation.selectedGroup !== "all" && navigation.selectedGroup !== "appearance"
    readonly property bool isAppearance: navigation.selectedGroup === "appearance" && navigation.query.length === 0
    AppearanceHub { anchors.fill:parent; anchors.margins:16; visible:root.isAppearance }
    C.ScrollView {
        visible:!root.isAppearance
        id: scroll
        anchors.fill: parent; anchors.margins: 24
        contentWidth: availableWidth
        C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 18
            RowLayout {
                Layout.fillWidth: true; spacing: 16
                HubIcon { symbol: navigation.query.length ? "search" : navigation.selectedGroup === "all" ? "all" : navigation.selectedGroup; implicitWidth: 42; implicitHeight: 42 }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 6
                    K.Heading { Layout.fillWidth: true; text: navigation.home ? i18n("Home") : navigation.title; level: 1; font.weight: Font.DemiBold; font.pointSize: K.Theme.defaultFont.pointSize * 1.9; wrapMode: Text.WordWrap }
                    C.Label { Layout.fillWidth: true; text: root.isAppearance ? i18n("A coordinated desktop, with every detail still in reach.") : navigation.description; wrapMode: Text.WordWrap; opacity: .75 }
                }
            }
            RowLayout {
                visible: navigation.home; Layout.fillWidth: true; spacing: 20
                ColumnLayout { Layout.fillWidth:true; spacing:10
                    K.Heading { Layout.fillWidth:true; text:i18n("Make this desktop yours"); level:2; font.weight:Font.DemiBold; wrapMode:Text.WordWrap }
                    C.Label { Layout.fillWidth:true; text:i18n("Personalize your look, set up your devices, and keep everything running the way you like."); wrapMode:Text.WordWrap; opacity:.78 }
                }
                DesktopPreview { visible: parent.width>500; Layout.preferredWidth:175; Layout.preferredHeight:110; miniature:true; material:appearancePreview.activeMaterial }
            }
            Loader { active: root.isCategory; visible: active; Layout.fillWidth:true; sourceComponent: Component { CategoryOverview {} } }
            GridLayout {
                visible: navigation.home
                Layout.fillWidth: true
                columns: width >= 700 ? 4 : width >= 400 ? 2 : 1; columnSpacing: 14; rowSpacing: 14
                Repeater {
                    model: navigation.home ? navigation.groups : []
                    C.AbstractButton {
                        id: category
                        objectName: "overviewCategory-" + modelData.id
                        required property var modelData
                        Layout.fillWidth: true; Layout.minimumWidth: 0
                        implicitHeight: Math.max(142, contentItem.implicitHeight + 32)
                        padding: 16; hoverEnabled: true
                        Accessible.name: modelData.name
                        onClicked: navigation.browse(modelData.id)
                        background: HubPanel { emphasized: category.activeFocus; tint: category.hovered ? .60 : .30 }
                        contentItem: ColumnLayout {
                            spacing: 10
                            RowLayout { Layout.fillWidth: true
                                HubIcon { symbol: category.modelData.id; implicitWidth: 30; implicitHeight: 30 }
                                Item { Layout.fillWidth: true }
                                C.Label { text: category.modelData.count; opacity: .65 }
                                K.Icon { source: "go-next"; implicitWidth: 16; implicitHeight: 16; opacity: .65 }
                            }
                            C.Label { Layout.fillWidth: true; text: category.modelData.name; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
                            C.Label { Layout.fillWidth: true; text: category.modelData.description; wrapMode: Text.WordWrap; opacity: .72; font.pointSize: K.Theme.smallFont.pointSize }
                        }
                    }
                }
            }
            C.Label { visible: !navigation.home && !root.isAppearance && !root.isCategory; text: navigation.query.length ? i18n("%1 matching settings", navigation.entries.length) : i18n("All %1 settings in this section", navigation.entries.length); font.bold: true }
            GridLayout {
                id: entries
                objectName: "settingsResults"
                visible: !navigation.home && !root.isCategory && !root.isAppearance
                Layout.fillWidth: true
                columns: width > 650 ? 2 : 1; columnSpacing: 10; rowSpacing: 10
                Repeater {
                    model: navigation.home || root.isCategory ? [] : navigation.entries
                    C.AbstractButton {
                        id: row
                        required property var modelData
                        Layout.fillWidth: true; Layout.minimumWidth: 0
                        implicitHeight: Math.max(76, contentItem.implicitHeight + 24); padding: 12; hoverEnabled: true
                        Accessible.name: modelData.name
                        Accessible.description: modelData.description
                        onClicked: navigation.open(modelData.id)
                        background: HubPanel { radius: 14; emphasized: row.activeFocus; tint: row.hovered ? .63 : .31 }
                        contentItem: RowLayout {
                            spacing: 12
                            K.Icon { source: row.modelData.icon; implicitWidth: 26; implicitHeight: 26 }
                            ColumnLayout { Layout.fillWidth: true; spacing: 4
                                C.Label { Layout.fillWidth: true; text: row.modelData.name; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
                                C.Label { Layout.fillWidth: true; text: row.modelData.description || row.modelData.groupName; wrapMode: Text.WordWrap; opacity: .7; font.pointSize: K.Theme.smallFont.pointSize }
                            }
                            K.Badge { visible: row.modelData.changedFromDefault && systemsettings.defaultsIndicatorsVisible; Layout.preferredWidth: 8; Layout.preferredHeight: 8; type: K.Badge.Type.Warning }
                            K.Icon { source: root.LayoutMirroring.enabled ? "go-previous" : "go-next"; implicitWidth: 16; implicitHeight: 16; opacity: .65 }
                        }
                    }
                }
            }
            C.Label { visible: !navigation.home && !navigation.entries.length; text: i18n("No matching settings. Try another search."); Layout.fillWidth: true; wrapMode: Text.WordWrap }
            C.Label { visible: navigation.home; text: i18n("%1 settings · One searchable catalog", navigation.catalog.length); opacity: .65; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        }
    }
}
