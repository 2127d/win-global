// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.Representation {
    Layout.minimumWidth: 320
    Layout.minimumHeight: 360
    Layout.preferredWidth: 432
    Layout.preferredHeight: 432
    collapseMarginsHint: true
    MediaPlayerView {
        anchors.fill: parent
        players: mpris2Model
    }
}
