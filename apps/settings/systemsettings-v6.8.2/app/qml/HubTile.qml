// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
C.AbstractButton {
    id: tile
    property string label
    property string detail
    property string symbol: "sliders"
    property bool horizontal: false
    property bool accented: false
    implicitHeight: Math.max(horizontal ? 76 : 112, contentItem.implicitHeight + 28)
    padding: 14
    hoverEnabled: true
    Accessible.name: label
    Accessible.description: detail
    background: HubPanel { emphasized: tile.activeFocus; accent: tile.accented; tint: tile.hovered ? .50 : .30; radius: 16 }
    contentItem: GridLayout {
        columns: tile.horizontal ? 3 : 1
        rowSpacing: 8; columnSpacing: 12
        HubIcon { symbol: tile.symbol; implicitWidth: tile.horizontal ? 27 : 30; implicitHeight: implicitWidth; color: tile.accented ? K.Theme.linkColor : K.Theme.textColor }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 4
            C.Label { Layout.fillWidth: true; text: tile.label; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
            C.Label { visible: tile.detail.length > 0; Layout.fillWidth: true; text: tile.detail; opacity: .76; font.pointSize: K.Theme.smallFont.pointSize; wrapMode: Text.WordWrap }
        }
        K.Icon { visible: tile.horizontal; source: "go-next"; implicitWidth: 16; implicitHeight: 16; opacity: .6 }
    }
}
