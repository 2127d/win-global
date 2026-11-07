// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as K
Item {
    implicitHeight: 176
    Accessible.ignored: true
    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter; width: Math.min(300,parent.width*.65); height: 28; radius: 14; y: 110; color: "#18003917"; rotation: -3 }
    Rectangle {
        id: battery
        anchors.centerIn: parent; width: Math.min(280,parent.width*.68); height: 110; radius: 48
        rotation: -7
        gradient: Gradient { GradientStop { position:0; color:"#eef9ec" } GradientStop { position:.13; color:"#70a870" } GradientStop { position:.34; color:"#edffe9" } GradientStop { position:.55; color:"#a5d998" } GradientStop { position:.83; color:"#457a46" } GradientStop { position:1; color:"#ddf1da" } }
        border.color: "#aaffffff"; border.width: 1.5
        Rectangle { anchors.right: parent.right; anchors.rightMargin: -12; anchors.verticalCenter: parent.verticalCenter; width: 17; height: 45; radius: 7; color: "#8aab8b"; border.color:"#b8d2b4"; z: -1 }
        Rectangle { anchors.fill: parent; anchors.margins: 9; radius: 42
            gradient: Gradient { GradientStop { position:0; color:"#b9ed67" } GradientStop { position:.26; color:"#efffd2" } GradientStop { position:.47; color:"#81cb28" } GradientStop { position:1; color:"#3d891c" } }
            border.width: 1; border.color: "#f0ffd0"
            Rectangle { x: 10; y: 8; width: parent.width-25; height: 17; radius: 10; color: "#70ffffff" }
            HubIcon { anchors.centerIn: parent; symbol:"power"; width: 52; height: 52; color: "white" }
        }
        Rectangle { width: 28; height: parent.height-4; anchors.verticalCenter: parent.verticalCenter; anchors.right: parent.right; anchors.rightMargin: 3; radius: 16; color:"#6678a78b"; border.color:"#99ebffeb" }
    }
    HubIcon { anchors.bottom: battery.bottom; x: battery.x + battery.width + 6; symbol:"leaf"; width: 45; height: 45; color: "#78aa3c"; rotation: 15 }
}
