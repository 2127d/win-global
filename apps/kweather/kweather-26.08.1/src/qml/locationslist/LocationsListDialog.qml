// Copyright 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

LocationDialog {
    id: root

    title: i18n("Locations")

    footer: Control {
        padding: 16
        contentItem: RowLayout {
            Item { Layout.fillWidth: true }
            AddLocationButton {
                onClicked: {
                    root.close();
                    applicationWindow().openAddLocation();
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    LocationsListView {
        onCloseRequested: root.close()
    }
}
