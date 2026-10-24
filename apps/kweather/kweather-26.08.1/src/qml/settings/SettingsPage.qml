/*
 * SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
 * SPDX-FileCopyrightText: 2020-2021 Devin Lin <espidev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

Kirigami.Page {
    id: root
    title: i18n("Settings")
    readonly property bool sequoiaAuxiliaryPage: true
    property alias flickable: scroll.contentItem
    background: Item {}
    padding: 0

    Basic.ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        topPadding: 8
        bottomPadding: 16
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        SettingsComponent {
            viewport: scroll.contentItem
            width: scroll.availableWidth
            onAboutRequested: applicationWindow().pageStack.push(aboutPage)
        }
    }
    Component {
        id: aboutPage
        Kirigami.Page {
            title: i18n("About Weather")
            readonly property bool sequoiaAuxiliaryPage: true
            padding: 0
            background: Item {}
            AboutView { anchors.fill: parent }
        }
    }
}
