// SPDX-License-Identifier: MIT
import QtQuick

Item {
    id: fader
    property Item clock
    property Item mainStack
    property Item footer
    property Item source
    property bool alwaysShowClock: true
    state: "on"
    // The wallpaper stays sharp, as on the login screen. Blur is local to cards.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#00000000" }
            GradientStop { position: 0.65; color: "#00000000" }
            GradientStop { position: 1; color: "#50080d13" }
        }
    }
    states: [
        State {
            name: "on"
            PropertyChanges { target: fader.mainStack; opacity: 1 }
            PropertyChanges { target: fader.footer; opacity: 1 }
            PropertyChanges { target: fader.clock; opacity: 1 }
        },
        State {
            name: "off"
            PropertyChanges { target: fader.mainStack; opacity: 0 }
            PropertyChanges { target: fader.footer; opacity: 0 }
            PropertyChanges { target: fader.clock; opacity: fader.alwaysShowClock ? 1 : 0 }
        }
    ]
    transitions: Transition {
        NumberAnimation { properties: "opacity"; duration: 250; easing.type: Easing.InOutQuad }
    }
}
