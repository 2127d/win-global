// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.sequoia.login 1.0

Controls.AbstractButton {
    id: control
    required property Item scene
    property string symbol: "shutdown"
    readonly property real circleSize: 34 * scene.powerUnit
    implicitWidth: 60 * scene.powerUnit
    implicitHeight: 56 * scene.powerUnit
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: text
    background: Item {
        WallpaperGlass {
            id: glass
            objectName: control.objectName + "Glass"
            anchors.horizontalCenter: parent.horizontalCenter
            width: control.circleSize; height: width
            source: control.scene.wallpaper
            sceneSize: Qt.size(control.scene.width, control.scene.height)
            // All ancestors used here are explicit binding dependencies, so the
            // sampled wallpaper follows Row layout and screen resize immediately.
            sampleRect: Qt.rect(control.parent.x + control.x + x, control.parent.y + control.y + y, width, height)
            blurRadius: 24 * control.scene.unit
            radius: width / 2
            tint: control.down ? "#80506670" : control.hovered ? "#7044545d" : "#70121d22"
            Rectangle {
                anchors.fill: parent
                color: "transparent"; radius: width / 2
                border.width: control.visualFocus ? 2 : 1
                border.color: control.visualFocus ? "#b8dfff" : "#607b929c"
            }
        }
    }
    contentItem: Item {
        opacity: control.enabled ? 1 : 0.4
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            y: (control.circleSize - height) / 2
            width: 20 * control.scene.powerUnit; height: width
            source: "../assets/" + control.symbol + ".svg"
            sourceSize.width: width; sourceSize.height: height
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: control.circleSize + 5 * control.scene.powerUnit
            text: control.text
            color: "white"; font.pixelSize: Math.max(10, 10 * control.scene.powerUnit)
        }
    }
}
