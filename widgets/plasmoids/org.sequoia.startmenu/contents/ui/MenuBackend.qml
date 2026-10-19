// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.private.kicker as Kicker
import org.kde.coreaddons as CoreAddons

Item {
    id: backend
    required property var configuration
    property var appletInterface: null
    property string query: ""
    readonly property alias categories: categories
    readonly property var apps: applications.count > 0 ? applications.modelForRow(0) : null
    readonly property alias favorites: favorites
    readonly property alias recent: recent
    readonly property var searchResults: search.count > 0 ? search.modelForRow(0) : null
    readonly property alias power: power
    readonly property string userName: user.fullName || user.loginName
    readonly property url avatar: user.faceIconUrl

    Kicker.RootModel {
        id: categories
        appletInterface: backend.appletInterface
        flat: true
        sorted: true
        showAllApps: false
        showAllAppsCategorized: false
        showRecentApps: false
        showRecentDocs: false
        showRecentFolders: false
        showPowerSession: false
        showFavoritesPlaceholder: false
        showRootSeparator: false
        favoritesModel: Kicker.SimpleFavoritesModel {
            id: favorites
            favorites: backend.configuration.favorites
        }
    }
    Kicker.RootModel {
        id: applications
        appletInterface: backend.appletInterface
        flat: true
        sorted: true
        showTopLevelItems: true
        showAllApps: true
        showRecentApps: false
        showRecentDocs: false
        showRecentFolders: false
        showPowerSession: false
        showFavoritesPlaceholder: false
        showRootSeparator: false
        favoritesModel: favorites
    }
    Kicker.RecentUsageModel {
        id: recent
        favoritesModel: favorites
        shownItems: backend.configuration.recommendationSource
        ordering: backend.configuration.recommendationOrder
    }
    Kicker.RunnerModel {
        id: search
        appletInterface: backend.appletInterface
        favoritesModel: favorites
        runners: ["krunner_services", "krunner_systemsettings", "baloosearch", "krunner_recentdocuments"]
        mergeResults: true
        query: backend.query
    }
    Kicker.SystemModel { id: power }
    CoreAddons.KUser { id: user }

    function isPinned(id) { return !!id && configuration.favorites.indexOf(id) !== -1; }
    function togglePin(id) {
        if (!id) return;
        const items = configuration.favorites.slice();
        const index = items.indexOf(id);
        if (index < 0) items.push(id); else items.splice(index, 1);
        configuration.favorites = items;
    }
    function movePin(id, offset) {
        const items = configuration.favorites.slice();
        const from = items.indexOf(id);
        const to = Math.max(0, Math.min(items.length - 1, from + offset));
        if (from < 0 || from === to) return;
        items.splice(to, 0, items.splice(from, 1)[0]);
        configuration.favorites = items;
    }
}
