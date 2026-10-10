// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.clock as PlasmaClock

Column {
    spacing: 6 * lockScreenUi.unit
    Text {
        text: Qt.formatTime(timeSource.dateTime, "HH:mm")
        color: "white"
        font.family: "Adwaita Sans"
        font.pixelSize: 54 * lockScreenUi.unit
        font.weight: Font.Light
        style: Text.Raised; styleColor: "#40000000"
    }
    Text {
        text: Qt.formatDate(timeSource.dateTime, "dddd, MMMM d")
        color: "white"
        font.family: "Adwaita Sans"
        font.pixelSize: 18 * lockScreenUi.unit
        style: Text.Raised; styleColor: "#40000000"
    }
    PlasmaClock.Clock { id: timeSource; trackSeconds: false }
}
