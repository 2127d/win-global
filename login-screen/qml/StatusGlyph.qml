// SPDX-License-Identifier: MIT
import QtQuick

Item {
    id: glyph
    property string symbol: ""
    property int batteryPercent: 0
    readonly property bool battery: symbol.indexOf("battery-") === 0
    Image {
        anchors.fill: parent
        visible: !glyph.battery
        source: glyph.symbol === "desktop-session" ? "../assets/status-session.svg" :
            glyph.symbol.indexOf("audio-volume-muted") === 0 ? "../assets/status-muted.svg" :
            glyph.symbol.indexOf("audio-volume-") === 0 ? "../assets/status-volume.svg" :
            glyph.symbol.indexOf("window-close") === 0 ? "../assets/status-close.svg" :
            glyph.symbol.indexOf("wired") >= 0 || glyph.symbol.indexOf("ethernet") >= 0 ? "../assets/status-wired.svg" : "../assets/status-wifi.svg"
        sourceSize.width: width; sourceSize.height: height
        opacity: glyph.symbol.indexOf("offline") >= 0 || glyph.symbol.indexOf("disconnected") >= 0 ? 0.45 : 1
    }
    Rectangle {
        visible: glyph.battery
        x: glyph.width * 0.04; y: glyph.height * 0.22
        width: glyph.width * 0.82; height: glyph.height * 0.56
        radius: glyph.height * 0.1; color: "transparent"; border.color: "#edf4f7"; border.width: 1.1
        Rectangle {
            x: glyph.width * 0.12; y: glyph.width * 0.12
            height: Math.max(0, parent.height - 2 * y)
            width: Math.max(0, parent.width - 2 * x) * Math.max(0, Math.min(100, glyph.batteryPercent)) / 100
            radius: 1
            color: glyph.batteryPercent <= 15 ? "#ffc2a1" : "white"
        }
        Rectangle { x: parent.width + 1; anchors.verticalCenter: parent.verticalCenter; width: glyph.width * 0.08; height: parent.height * 0.45; radius: 1; color: "white" }
    }
}
