// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.kicker as Kicker

ColumnLayout {
    id: settings
    property var cfg_favorites: []
    function move(from, offset) {
        const items = cfg_favorites.slice();
        const to = from + offset;
        if (to < 0 || to >= items.length) return;
        items.splice(to, 0, items.splice(from, 1)[0]);
        cfg_favorites = items;
    }
    function remove(index) {
        const items = cfg_favorites.slice();
        items.splice(index, 1);
        cfg_favorites = items;
    }
    Kirigami.Heading { text: qsTr("Your pinned apps"); level: 2 }
    QQC2.Label { text: qsTr("Use the arrows to arrange apps. You can also pin and unpin apps directly in the Start menu with a right-click."); Layout.fillWidth: true; wrapMode: Text.Wrap }
    Kicker.RootModel {
        showAllApps: false; showRecentApps: false; showRecentDocs: false; showPowerSession: false
        favoritesModel: Kicker.SimpleFavoritesModel { id: favoriteNames; favorites: settings.cfg_favorites }
    }
    QQC2.ScrollView {
        Layout.fillWidth: true; Layout.fillHeight: true
        implicitHeight: 320
        contentWidth: availableWidth
        ListView {
            id: pinnedList
            model: favoriteNames
            spacing: 5; clip: true
            delegate: RowLayout {
                required property int index
                required property var model
                readonly property int pinIndex: settings.cfg_favorites.indexOf(model.favoriteId)
                width: pinnedList.width
                Kirigami.Icon { source: model.decoration; Layout.preferredWidth: 28; Layout.preferredHeight: 28 }
                QQC2.Label { text: model.display; elide: Text.ElideRight; Layout.fillWidth: true }
                QQC2.ToolButton { icon.name: "go-up"; enabled: pinIndex > 0; Accessible.name: qsTr("Move earlier"); onClicked: settings.move(pinIndex, -1) }
                QQC2.ToolButton { icon.name: "go-down"; enabled: pinIndex >= 0 && pinIndex < settings.cfg_favorites.length - 1; Accessible.name: qsTr("Move later"); onClicked: settings.move(pinIndex, 1) }
                QQC2.ToolButton { icon.name: "list-remove"; enabled: pinIndex >= 0; Accessible.name: qsTr("Unpin app"); onClicked: settings.remove(pinIndex) }
            }
        }
    }
    QQC2.Label { visible: settings.cfg_favorites.length === 0; text: qsTr("No pinned apps. Pin apps from the menu to get started.") }
    QQC2.Button {
        text: qsTr("Restore suggested pins")
        icon.name: "edit-undo"
        onClicked: settings.cfg_favorites = ["preferred://browser", "org.kde.dolphin.desktop", "org.kde.konsole.desktop", "systemsettings.desktop", "org.kde.kate.desktop", "org.kde.discover.desktop"]
    }
}
