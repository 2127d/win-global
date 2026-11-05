/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

DiscoverPage {
    id: page
    sequoiaTitleInset: leftPadding + homeContent.x + 8

    title: i18nc("@title:window the name of a top-level 'home' page", "Home")
    objectName: "featured"

    actions: window.wideScreen ? [ searchAction ] : []

    header: DiscoverInlineMessage {
        id: message

        inlineMessage: Discover.ResourcesModel.inlineMessage ? Discover.ResourcesModel.inlineMessage : app.homePageMessage
    }

    readonly property bool isHome: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    DiscoverApp.FeaturedModel {
        id: featuredModel
    }

    Kirigami.LoadingPlaceholder {
        visible: featuredModel.isFetching
        anchors.centerIn: parent
    }

    Loader {
        active: !featuredModel.isFetching && [featuredModel, popularSection, recentSection, gamesSection, devSection].every((model) => model.count === 0)

        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        sourceComponent: Kirigami.PlaceholderMessage {
            readonly property Discover.InlineMessage helpfulError: featuredModel.currentApplicationBackend?.explainDysfunction() ?? null

            icon.name: helpfulError?.iconName ?? ""
            text: i18n("Unable to load applications")
            explanation: helpfulError?.message ?? ""

            Repeater {
                model: helpfulError?.actions ?? null
                delegate: QQC2.Button {
                    id: delegate

                    required property Discover.DiscoverAction modelData

                    Layout.alignment: Qt.AlignHCenter

                    action: ConvertDiscoverAction {
                        action: delegate.modelData
                    }
                }
            }
        }
    }

    signal clearSearch()
    property bool sequoiaShowcaseHome: true


    Item {
        implicitHeight: homeContent.implicitHeight
        ColumnLayout {
            id: homeContent
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, 1480)
            spacing: 30
            visible: !featuredModel.isFetching

            HomeSection {
                Layout.fillWidth: true
                title: i18nc("@title:group", "Editor’s Choice")
                model: featuredModel
                showcase: true
                onShowAll: window.pageStack.push(Qt.resolvedUrl("HomeCollectionPage.qml"))
                onRevealRequested: item => page.ensureVisible(item)
            }
            HomeSection {
                id: popularSection
                Layout.fillWidth: true
                title: i18nc("@title:group", "Most Popular")
                model: DiscoverApp.LimitedRowCountProxyModel {
                    pageSize: popularSection.columns * 2
                    sourceModel: DiscoverApp.OdrsAppsModel {}
                }
                onShowAll: Navigation.openCategory(Discover.CategoryModel.findCategoryByName("All Applications"))
                onRevealRequested: item => page.ensureVisible(item)
            }
            HomeSection {
                id: recentSection
                Layout.fillWidth: true
                title: i18nc("@title:group", "Newly Published & Recently Updated")
                model: recentlyUpdatedModelInstantiator.object
                onShowAll: Navigation.openApplicationList({
                    category: Discover.CategoryModel.findCategoryByName("All Applications"),
                    sortRole: Discover.ResourcesProxyModel.ReleaseDateRole,
                    sortOrder: Qt.DescendingOrder
                })
                onRevealRequested: item => page.ensureVisible(item)
            }
            HomeSection {
                id: gamesSection
                Layout.fillWidth: true
                title: i18nc("@title:group", "Highest-Rated Games")
                showcase: true
                model: DiscoverApp.LimitedRowCountProxyModel {
                    pageSize: 6
                    sourceModel: Discover.ResourcesProxyModel {
                        filteredCategoryName: "Games"
                        backendFilter: Discover.ResourcesModel.currentApplicationBackend
                        sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                        sortOrder: Qt.DescendingOrder
                    }
                }
                onShowAll: Navigation.openApplicationList({
                    category: Discover.CategoryModel.findCategoryByName("Games"),
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole,
                    sortOrder: Qt.DescendingOrder
                })
                onRevealRequested: item => page.ensureVisible(item)
            }
            HomeSection {
                id: devSection
                Layout.fillWidth: true
                title: i18nc("@title:group", "Highest-Rated Developer Tools")
                model: DiscoverApp.LimitedRowCountProxyModel {
                    pageSize: devSection.columns * 2
                    sourceModel: Discover.ResourcesProxyModel {
                        filteredCategoryName: "Development"
                        backendFilter: Discover.ResourcesModel.currentApplicationBackend
                        sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                        sortOrder: Qt.DescendingOrder
                    }
                }
                onShowAll: Navigation.openApplicationList({
                    category: Discover.CategoryModel.findCategoryByName("Development"),
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole,
                    sortOrder: Qt.DescendingOrder
                })
                onRevealRequested: item => page.ensureVisible(item)
            }
        }
    }
    Instantiator {
        id: recentlyUpdatedModelInstantiator
        active: ["flatpak-backend", "snap-backend"].includes(Discover.ResourcesModel.currentApplicationBackend?.name ?? "")
        DiscoverApp.LimitedRowCountProxyModel {
            pageSize: recentSection.columns * 2
            sourceModel: Discover.ResourcesProxyModel {
                filteredCategoryName: "All Applications"
                backendFilter: Discover.ResourcesModel.currentApplicationBackend
                sortRole: Discover.ResourcesProxyModel.ReleaseDateRole
                sortOrder: Qt.DescendingOrder
            }
        }
    }
}
