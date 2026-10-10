// SPDX-License-Identifier: MIT
import QtQuick

Rectangle {
    id: root
    property int stage: 0
    color: "#202124"
    Column {
        anchors.centerIn: parent
        spacing: 24
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 64; height: 64; radius: 19
            gradient: Gradient {
                GradientStop { position: 0; color: "#a9c8ff" }
                GradientStop { position: 1; color: "#5085de" }
            }
            Row {
                anchors.centerIn: parent
                spacing: 5
                Repeater {
                    model: 3
                    Rectangle { required property int index; width: 6; height: 20 + index * 6; radius: 3; color: "#f1f5ff"; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
        Text { text: "Sequoia Eleven"; color: "#f1f3f7"; font.family: "Nunito"; font.pixelSize: 22; font.weight: Font.Medium }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 116; height: 3; radius: 1.5; color: "#34373e"
            Rectangle {
                width: parent.width * Math.min(1, Math.max(0.08, root.stage / 6))
                height: parent.height; radius: parent.radius; color: "#9ebfff"
                Behavior on width { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
            }
        }
    }
}
