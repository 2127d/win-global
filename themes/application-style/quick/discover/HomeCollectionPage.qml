// SPDX-License-Identifier: LGPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami

DiscoverPage {
    id: page
    title: i18nc("@title:group", "Editor’s Choice")
    signal clearSearch()
    DiscoverApp.FeaturedModel { id: featured }
    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
        visible: featured.isFetching
    }
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        visible: !featured.isFetching && featured.count === 0
        text: i18n("Unable to load applications")
    }
    GridLayout {
        width: page.availableWidth
        columns: Math.max(1, Math.min(4, Math.floor((width + 16) / 280)))
        rowSpacing: 16
        columnSpacing: 16
        Repeater {
            model: featured
            delegate: HomeAppCard {
                required property var model
                application: model.application
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.minimumWidth: 0
                onRevealRequested: item => page.ensureVisible(item)
            }
        }
    }
}
