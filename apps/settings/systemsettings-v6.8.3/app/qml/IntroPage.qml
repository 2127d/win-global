// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
Loader {
    objectName: "settingsOverviewLoader"
    source: appearancePreview.enabled ? "HubOverview.qml" : "LegacyIntroPage.qml"
}
