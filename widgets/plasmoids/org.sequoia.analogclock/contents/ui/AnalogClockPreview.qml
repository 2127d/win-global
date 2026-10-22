// SPDX-License-Identifier: MIT
import QtQuick

Item {
    property var presetConfiguration: ({faceStyle: "sequoia"})
    ClockFace {
        anchors.fill: parent
        anchors.margins: 6
        rounded: parent.presetConfiguration.faceStyle !== "original"
    }
}
