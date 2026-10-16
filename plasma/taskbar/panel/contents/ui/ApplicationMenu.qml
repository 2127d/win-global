// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.plasma.private.kicker as Kicker
import org.kde.kirigami as Kirigami

PlasmaCore.Dialog {
    id: menu
    objectName: "integratedApplicationMenu"
    readonly property int applicationCount: apps.count
    required property var appletInterface
    signal settingsRequested()
    type: PlasmaCore.Dialog.PopupMenu
    hideOnWindowDeactivate: true
    backgroundHints: PlasmaCore.Dialog.StandardBackground
    visible: false
    onVisibleChanged: if (visible) {
        search.text = "";
        Qt.callLater(() => { menu.requestActivate(); search.forceActiveFocus(); });
    }
    readonly property Kicker.RootModel applicationModel: Kicker.RootModel {
        id: applications
        appletInterface: menu.appletInterface
        flat: true
        sorted: true
        showAllApps: true
        showTopLevelItems: true
        showRecentApps: false
        showRecentDocs: false
        showRecentFolders: false
        showPowerSession: false
        showFavoritesPlaceholder: false
        showRootSeparator: false
    }
    readonly property Kicker.RunnerModel searchModel: Kicker.RunnerModel {
        id: results
        appletInterface: menu.appletInterface
        runners: ["krunner_services"]
        mergeResults: true
        query: search.text
    }
    mainItem: ColumnLayout {
        width: Kirigami.Units.gridUnit * 22
        height: Kirigami.Units.gridUnit * 26
        spacing: Kirigami.Units.smallSpacing
        Keys.onEscapePressed: menu.visible = false
        PC3.TextField {
            id: search
            Layout.fillWidth: true
            placeholderText: i18n("Search applications…")
            Accessible.name: placeholderText
            Keys.onDownPressed: { apps.forceActiveFocus(); apps.currentIndex = 0; }
            onAccepted: apps.launch(Math.max(0, apps.currentIndex))
        }
        PC3.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListView {
                id: apps
                clip: true
                model: search.text.length > 0
                    ? (results.count > 0 ? results.modelForRow(0) : null)
                    : (applications.count > 0 ? applications.modelForRow(0) : null)
                function launch(row) {
                    if (model && row < count && model.trigger(row, "", null)) menu.visible = false;
                }
                Keys.onReturnPressed: launch(currentIndex)
                Keys.onEnterPressed: launch(currentIndex)
                delegate: PC3.ItemDelegate {
                    id: application
                    required property int index
                    required property var model
                    width: ListView.view.width
                    text: model.display || ""
                    contentItem: RowLayout {
                        Kirigami.Icon { source: application.model.decoration; Layout.preferredWidth: 24; Layout.preferredHeight: 24 }
                        PC3.Label { text: application.text; Layout.fillWidth: true; elide: Text.ElideRight }
                    }
                    onClicked: apps.launch(index)
                }
            }
        }
        PC3.Button {
            Layout.fillWidth: true
            text: i18n("Taskbar Settings…")
            icon.name: "configure"
            onClicked: { menu.visible = false; menu.settingsRequested(); }
        }
    }
}
