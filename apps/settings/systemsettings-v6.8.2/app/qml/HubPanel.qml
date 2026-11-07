// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as Kirigami
import org.sequoia.settingsglass 1.0 as Glass
Rectangle {
    id: panel
    property bool followMaterial: true
    property bool accent: false
    property bool emphasized: false
    property real tint: 0.42
    radius: 18
    readonly property real surfaceOpacity: followMaterial ? (appearancePreview.activeMaterial === "solid" ? 1 : .78 + .20 * tint) : tint
    color: Qt.alpha(accent ? Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.highlightColor, .18)) : Kirigami.Theme.backgroundColor, surfaceOpacity)
    border.width: emphasized ? 1.5 : 1
    border.color: emphasized ? Qt.alpha(Kirigami.Theme.highlightColor, 0.65) : Qt.alpha(Kirigami.Theme.textColor, .10)
    Glass.IslandRim { anchors.fill: parent; radius: panel.radius; thickness: panel.followMaterial ? 3 : 1 }
}
