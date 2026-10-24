/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2022 Devin Lin <espidev@gmail.com>
 * SPDX-FileCopyrightText: 2021 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    title: i18n("Locations")
    readonly property bool sequoiaAuxiliaryPage: true
    background: Item {}
    padding: 8

    property int currentIndex: 0

    footer: Control {
        padding: 12
        background: null
        contentItem: RowLayout {
            Item { Layout.fillWidth: true }
            AddLocationButton { onClicked: applicationWindow().openAddLocation() }
            Item { Layout.fillWidth: true }
        }
    }

    LocationsListView {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
    }
}
