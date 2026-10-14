import QtQuick

Rectangle {
    id: sample
    required property int category
    required property int entryDuration
    required property int distance
    required property string curve
    required property bool reduced
    required property real playbackScale
    property real progress: 0
    property bool opened: false
    readonly property real travel: reduced ? 0 : distance
    readonly property real pulse: reduced || !opened ? 0 : Math.sin(progress * Math.PI)
    readonly property int enterMs: Math.round((reduced ? Math.min(entryDuration, 80) : entryDuration) * playbackScale)
    readonly property int exitMs: Math.round(enterMs * (category === 3 || category === 5 ? 1 : .7))
    readonly property int enterCurve: category === 3 || category === 5 ? Easing.InOutCubic : curve === "OutQuint" ? Easing.OutQuint : Easing.OutCubic
    color: "#122039"
    radius: 16
    clip: true
    activeFocusOnTab: true
    Accessible.name: "Animation sample. Press Space to toggle."
    Accessible.role: Accessible.Button
    border.color: activeFocus ? "#6b9ff8" : "#283854"
    border.width: activeFocus ? 2 : 1
    function reset() { tween.stop(); opened = false; progress = 0; }
    function play() {
        tween.stop();
        opened = !opened;
        tween.from = progress;
        tween.to = opened ? 1 : 0;
        tween.duration = opened ? enterMs : exitMs;
        tween.easing.type = opened ? enterCurve : category === 3 || category === 5 ? Easing.InOutCubic : Easing.InCubic;
        tween.start();
    }
    onCategoryChanged: reset()
    onReducedChanged: reset()
    NumberAnimation { id: tween; target: sample; property: "progress" }
    Keys.onSpacePressed: play()
    Keys.onReturnPressed: play()
    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { sample.forceActiveFocus(); sample.play(); } }

    component Caption: Text { color: "#e8eefb"; font.pixelSize: 13 }
    component Line: Rectangle { color: "#3c4d68"; radius: 3; height: 6 }

    // A color transition, toggle, and restrained button press share the control family.
    Item {
        anchors.centerIn: parent; width: 280; height: 155; visible: sample.category === 0
        Caption { text: "Wi-Fi"; x: 10; y: 16; font.pixelSize: 18 }
        Rectangle {
            x: 202; y: 12; width: 62; height: 32; radius: 16
            color: Qt.rgba(.21 + .08 * sample.progress, .28 + .25 * sample.progress, .40 + .50 * sample.progress, 1)
            Rectangle { x: 4 + (sample.reduced ? (sample.opened ? 30 : 0) : 30 * sample.progress); y: 4; width: 24; height: 24; radius: 12; color: "white" }
        }
        Rectangle {
            x: 10; y: 75; width: 254; height: 52; radius: 10
            color: "#4d86e8"; opacity: .72 + sample.progress * .28
            scale: sample.reduced ? 1 : 1 - .035 * sample.pulse
            Caption { anchors.centerIn: parent; text: sample.opened ? "Connected" : "Connect"; font.weight: Font.DemiBold }
        }
    }
    Item {
        anchors.centerIn: parent; width: 270; height: 130; visible: sample.category === 1
        Rectangle { x: 0; y: 64; width: 270; height: 66; radius: 20; color: "#2a3a54" }
        Repeater {
            model: 4
            Rectangle {
                required property int index
                x: 19 + index * 62; y: 75 - (index === 1 ? sample.pulse * sample.travel : 0)
                width: 46; height: 44; radius: 11
                color: ["#629afb", "#eab95d", "#a69cf5", "#6ac8b2"][index]
                scale: index === 1 && !sample.reduced ? 1 + sample.pulse * .14 : 1
                Caption { anchors.centerIn: parent; text: ["F", "D", "S", "M"][index]; color: "#142137"; font.pixelSize: 21; font.bold: true }
            }
        }
        Rectangle { x: 100; y: 122; width: 5; height: 3; radius: 1; color: "#ffffff"; opacity: sample.progress }
        Caption { anchors.horizontalCenter: parent.horizontalCenter; y: 10; text: sample.opened ? "Application running" : "Launch an application" }
    }
    Item {
        anchors.fill: parent; anchors.margins: 20; visible: sample.category === 2
        Rectangle { width: parent.width; height: 29; radius: 7; color: "#2b3b57"; Caption { x: 12; y: 7; text: "Plasma       Desktop       Go"; font.pixelSize: 11 } }
        Rectangle {
            x: 16; y: 54 - sample.travel * (1 - sample.progress); width: parent.width - 32; height: 136
            radius: 13; color: "#eef2f9"; opacity: sample.progress
            Caption { x: 16; y: 17; text: "Restart Plasma Shell?"; color: "#19273d"; font.bold: true }
            Caption { x: 16; y: 43; width: parent.width - 32; text: "Your application windows stay open."; wrapMode: Text.WordWrap; color: "#55637a"; font.pixelSize: 12 }
            Rectangle { x: parent.width - 107; y: 89; width: 91; height: 30; radius: 7; color: "#4b80da"; Caption { anchors.centerIn: parent; text: "Restart"; font.pixelSize: 11 } }
            Caption { x: parent.width - 160; y: 97; text: "Cancel"; color: "#46556d"; font.pixelSize: 11 }
        }
    }
    Item {
        anchors.fill: parent; anchors.margins: 24; visible: sample.category === 3
        Rectangle {
            width: 83; height: parent.height; radius: 10; color: "#263650"
            Rectangle { x: 7; y: 12 + (sample.reduced ? (sample.opened ? 38 : 0) : 38 * sample.progress); width: 69; height: 30; radius: 6; color: "#4d79c5" }
            Caption { x: 13; y: 20; text: "General"; font.pixelSize: 11 }
            Caption { x: 13; y: 58; text: "Display"; font.pixelSize: 11 }
        }
        Item {
            x: 99; width: parent.width - 99; height: parent.height; clip: true
            Item {
                width: parent.width; height: parent.height; x: -sample.travel * 2 * sample.progress; opacity: 1 - sample.progress
                Caption { y: 17; text: "General"; font.pixelSize: 18 }
                Line { y: 59; width: parent.width * .8 }
                Line { y: 82; width: parent.width * .6 }
                Line { y: 105; width: parent.width * .75 }
            }
            Item {
                width: parent.width; height: parent.height; x: sample.travel * 2 * (1 - sample.progress); opacity: sample.progress
                Caption { y: 17; text: "Display"; font.pixelSize: 18 }
                Rectangle { y: 58; width: parent.width - 10; height: 83; radius: 9; color: "#4c79bd" }
                Line { y: 163; width: parent.width * .6 }
            }
        }
    }
    Item {
        anchors.fill: parent; anchors.margins: 20; visible: sample.category === 4
        Rectangle { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; width: 134; height: 22; radius: 8; color: "#33435e" }
        Rectangle {
            x: 16; y: 16 + sample.travel * (1 - sample.progress); width: parent.width - 32; height: parent.height - 53
            radius: 12; color: "#eef2f9"; opacity: sample.progress
            scale: sample.reduced ? 1 : .93 + .07 * sample.progress
            Rectangle { x: 0; y: 0; width: parent.width; height: 31; radius: 12; color: "#d6dfed" }
            Caption { x: 14; y: 9; text: "Dolphin"; color: "#2b3c57"; font.pixelSize: 11 }
            Repeater {
                model: 6
                Rectangle { required property int index; x: 22 + (index % 3) * 72; y: 53 + Math.floor(index / 3) * 47; width: 44; height: 30; radius: 5; color: index % 2 ? "#719fe2" : "#e6bb70" }
            }
        }
    }
    Item {
        anchors.fill: parent; anchors.margins: 22; visible: sample.category === 5; clip: true
        Repeater {
            model: 2
            Rectangle {
                required property int index
                x: sample.reduced ? 0 : (index - sample.progress) * parent.width
                width: parent.width; height: parent.height - 25; radius: 12
                color: index ? "#304969" : "#414366"
                opacity: sample.reduced ? (index ? sample.progress : 1 - sample.progress) : 1
                Caption { x: 15; y: 15; text: index ? "Desktop 2 · Focus" : "Desktop 1 · Everyday" }
                Rectangle { x: 16; y: 50; width: parent.width * .53; height: 102; radius: 8; color: index ? "#608990" : "#7474a3" }
                Rectangle { x: parent.width * .56; y: 76; width: parent.width * .35; height: 91; radius: 8; color: index ? "#c2a373" : "#7697bd" }
            }
        }
        Row { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; spacing: 7
            Rectangle { width: 7; height: 7; radius: 4; color: sample.opened ? "#4d5c75" : "#b5cff9" }
            Rectangle { width: 7; height: 7; radius: 4; color: sample.opened ? "#b5cff9" : "#4d5c75" }
        }
    }
    Item {
        anchors.centerIn: parent; width: 240; height: 150; visible: sample.category === 6
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 0; width: 74; height: 74; radius: 37; color: "transparent"; border.color: "#7edcc0"; border.width: 2; scale: 1 + sample.pulse * .32; opacity: sample.pulse }
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 6; width: 62; height: 62; radius: 31; color: sample.opened ? "#70ceb3" : "#374b68"
            Caption { anchors.centerIn: parent; text: sample.opened ? "✓" : "…"; font.pixelSize: 27; color: "#14283c" }
        }
        Caption { anchors.horizontalCenter: parent.horizontalCenter; y: 91; text: sample.opened ? "Changes saved" : "Ready to save"; font.pixelSize: 16 }
        Caption { anchors.horizontalCenter: parent.horizontalCenter; y: 122; text: "One cue, then still."; color: "#9eafca"; font.pixelSize: 12 }
    }
}
