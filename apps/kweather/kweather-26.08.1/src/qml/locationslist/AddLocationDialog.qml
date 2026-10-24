// Copyright 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

LocationDialog {
    id: root

    title: i18n("Add Location")
    closeButtonObjectName: "weatherCloseLocationSearch"
    // Keep the modal steady while results change; its list handles overflow.
    height: Math.min(440, Math.max(0, parent.height - 48))
    width: Math.min(512, Math.max(0, parent.width - 48))
    // There are no footer actions here; the native scroll separator would span
    // the rounded lower corners when search results overflow.
    footer: Item { implicitHeight: 0 }

    onOpened: addLocationListView.focusRequested()

    contentItem: AddLocationListView {
        id: addLocationListView
        clip: true
        onCloseRequested: root.close()
    }
}
