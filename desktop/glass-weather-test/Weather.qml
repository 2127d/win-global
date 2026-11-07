// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    id: root
    width: 240; height: 328
    minimumWidth: 240; maximumWidth: 240
    minimumHeight: 328; maximumHeight: 328
    color: "transparent"
    title: "Liquid Glass — Weather Test"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    // Fixed reference data; this prototype does not make weather requests.
    Rectangle {
        anchors.fill: parent
        radius: 40
        gradient: Gradient {
            GradientStop { position: 0; color: "#10ffffff" }
            GradientStop { position: 0.32; color: "#04374049" }
            GradientStop { position: 1; color: "#14333b46" }
        }
        border.width: 1
        border.color: "#70ffffff"
        antialiasing: true
    }
    Rectangle {
        anchors.fill: parent; anchors.margins: 2
        radius: 38; color: "transparent"
        border.width: 1; border.color: "#18ffffff"
        antialiasing: true
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter; y: 20
        text: "Toronto"; color: "white"; font.family: "Noto Sans"
        font.pixelSize: 18; font.weight: Font.DemiBold
        style: Text.Raised; styleColor: "#50232a32"
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter; y: 46
        text: "Today"; color: "#f5ffffff"; font.family: "Noto Sans"; font.pixelSize: 11
    }
    Item {
        x: 58; y: 77; width: 124; height: 84
        Rectangle {
            x: 66; y: 0; width: 44; height: 44; radius: 22
            gradient: Gradient {
                GradientStop { position: 0; color: "#ffe485" }
                GradientStop { position: 1; color: "#ffc341" }
            }
            antialiasing: true
        }
        // Two softly layered clouds; labels and artwork stay outside the blur.
        Item {
            x: 36; y: 19; opacity: 0.95
            Rectangle { x: 6; y: 1; width: 39; height: 39; radius: 20; color: "#e3edf0"; antialiasing: true }
            Rectangle { x: 28; y: 16; width: 38; height: 33; radius: 17; color: "#e3edf0"; antialiasing: true }
            Rectangle { x: 0; y: 21; width: 49; height: 29; radius: 14; color: "#e3edf0"; antialiasing: true }
        }
        Canvas {
            x: 0; y: 26; width: 99; height: 56
            onPaint: {
                const c=getContext("2d"); c.clearRect(0,0,width,height);
                const g=c.createLinearGradient(0,0,0,56);
                g.addColorStop(0,"#ffffff"); g.addColorStop(1,"#cedde2");
                c.fillStyle=g; c.beginPath();
                c.moveTo(23,53); c.bezierCurveTo(1,53,1,26,20,23);
                c.bezierCurveTo(19,-1,57,-4,65,20);
                c.bezierCurveTo(89,17,101,52,76,53);
                c.closePath(); c.fill();
            }
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter; y: 169
        text: "21°"; color: "white"; font.family: "Noto Sans"
        font.pixelSize: 40; font.weight: Font.Medium
        style: Text.Raised; styleColor: "#50232a32"
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter; y: 221
        text: "Mostly Cloudy"; color: "white"; font.family: "Noto Sans"; font.pixelSize: 12
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter; y: 268; spacing: 17
        Repeater {
            model: [{kind: "wind", value: "4 km/h"}, {kind: "rain", value: "45%"}, {kind: "drop", value: "56%"}]
            delegate: Column {
                required property var modelData
                spacing: 6; width: 44
                Canvas {
                    anchors.horizontalCenter: parent.horizontalCenter; width: 22; height: 21
                    onPaint: {
                        const c=getContext("2d"); c.clearRect(0,0,width,height);
                        c.lineWidth=1.7; c.lineCap="round"; c.strokeStyle="#f2ffffff";
                        if(modelData.kind==="wind") {
                            c.beginPath();c.moveTo(1,7);c.lineTo(14,7);c.bezierCurveTo(21,7,21,0,16,1);c.stroke();
                            c.beginPath();c.moveTo(3,11);c.lineTo(19,11);c.stroke();
                            c.beginPath();c.moveTo(1,15);c.lineTo(12,15);c.bezierCurveTo(18,15,17,21,12,20);c.stroke();
                        } else if(modelData.kind==="drop") {
                            const g=c.createLinearGradient(0,0,0,21);g.addColorStop(0,"#c9f1ff");g.addColorStop(1,"#5798ef");
                            c.fillStyle=g;c.beginPath();c.moveTo(11,1);c.bezierCurveTo(10,6,3,10,4,15);c.bezierCurveTo(4,23,20,23,18,14);c.bezierCurveTo(17,9,12,5,11,1);c.fill();
                        } else {
                            c.fillStyle="#dce8ef";c.beginPath();c.moveTo(5,11);c.bezierCurveTo(0,11,0,5,5,5);c.bezierCurveTo(5,-1,16,-1,17,5);c.bezierCurveTo(24,5,23,12,17,12);c.closePath();c.fill();
                            c.strokeStyle="#7cbbff";c.beginPath();c.moveTo(8,15);c.lineTo(6,19);c.moveTo(15,15);c.lineTo(13,19);c.stroke();
                        }
                    }
                }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.value; color: "white"; font.pixelSize: 11; font.family: "Noto Sans" }
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: mouse => {
            if (mouse.button === Qt.LeftButton) root.startSystemMove();
            else menu.popup();
        }
    }
    Shortcut { sequence: "Escape"; onActivated: Qt.quit() }
    Menu {
        id: menu
        MenuItem { text: "Weather test · sample data"; enabled: false }
        MenuSeparator {}
        MenuItem { text: "Close test"; onTriggered: Qt.quit() }
    }
}
