// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami.platform as Platform
import org.kde.kirigami.primitives as Primitives

Item {
    id: tile
    objectName: "sequoiaSidebarIcon"
    property var source
    property bool selected: false
    property bool hovered: false
    readonly property bool dark: Platform.Theme.textColor.hslLightness > 0.5
    implicitWidth: 26
    implicitHeight: 26
    readonly property color accent: {
        const name = String(source).toLowerCase()
        if (/go-home/.test(name)) return "#377eea"
        if (/view-list-details/.test(name)) return "#169c8b"
        if (/update|refresh/.test(name)) return "#dc7729"
        if (/configure|settings/.test(name)) return "#68798e"
        if (/help|info/.test(name)) return "#7060cb"
        if (/accessibility/.test(name)) return "#3c8bd9"
        if (/development/.test(name)) return "#7961cf"
        if (/education/.test(name)) return "#d68a2a"
        if (/games/.test(name)) return "#d34b80"
        if (/graphics/.test(name)) return "#a257cb"
        if (/internet|network/.test(name)) return "#2a94cd"
        if (/multimedia|audio|video/.test(name)) return "#d35f56"
        if (/office/.test(name)) return "#bd8a30"
        if (/science|math/.test(name)) return "#31a38e"
        if (/system/.test(name)) return "#61738c"
        if (/utilities/.test(name)) return "#55879b"
        if (/font/.test(name)) return "#c6656c"
        if (/hardware|driver|peripheral|computer/.test(name)) return "#428a92"
        if (/plasma/.test(name)) return "#4c9bc5"
        return "#8055cf"
    }

    transform: Translate {
        y: tile.hovered && !tile.selected ? -1 : 0
        Behavior on y {
            NumberAnimation { duration: Platform.Units.longDuration > 0 ? 160 : 0; easing.type: Easing.OutCubic }
        }
    }
    Rectangle {
        anchors.fill: parent
        radius: width * 0.38
        color: tile.selected ? tile.accent : Qt.alpha(tile.accent, tile.dark ? 0.19 : 0.13)
        border.width: 1
        border.color: Qt.alpha(tile.accent, tile.selected ? 0 : 0.16)
        Behavior on color {
            ColorAnimation { duration: Platform.Units.longDuration > 0 ? 160 : 0; easing.type: Easing.OutCubic }
        }
    }
    Primitives.Icon {
        anchors.centerIn: parent
        width: Math.round(parent.width * 0.69)
        height: width
        source: tile.source
        color: tile.selected ? "white" : tile.dark ? Qt.lighter(tile.accent, 1.45) : tile.accent
        isMask: true
        selected: false
        animated: false
    }
}
