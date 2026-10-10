// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Effects
import org.kde.ksvg as KSvg

Rectangle {
    width: 800; height: 440
    color: "#202124"
    Row {
        Repeater {
            model: ["light", "dark"]
            delegate: Item {
                id: pane
                required property string modelData
                width: 400; height: 440
                readonly property color ink: modelData === "light" ? "#24262b" : "#f2f3f5"
                readonly property string asset: projectRoot + "/plasma/desktoptheme/sequoia-eleven-" + modelData + "/widgets/background.svg"
                Item {
                    id: wallpaper
                    anchors.fill: parent
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0; color: "#68bcc8" }
                            GradientStop { position: 0.48; color: "#a288ad" }
                            GradientStop { position: 1; color: "#385284" }
                        }
                    }
                    Rectangle { x: -50; y: 230; width: 490; height: 90; rotation: -28; radius: 45; color: "#a5e2e0" }
                    Rectangle { x: 270; y: -60; width: 80; height: 480; rotation: 28; radius: 40; color: "#505d9f" }
                }
                Text {
                    x: 38; y: 22
                    text: pane.modelData === "light" ? "FROSTED LIGHT" : "FROSTED DARK"
                    color: "white"; font.family: "Nunito"; font.pixelSize: 13; font.letterSpacing: 2
                }
                Item {
                    id: widget
                    x: 38; y: 60; width: 324; height: 346
                    ShaderEffectSource {
                        id: wallpaperCrop
                        sourceItem: wallpaper
                        sourceRect: Qt.rect(widget.x, widget.y, widget.width, widget.height)
                        visible: false
                    }
                    KSvg.FrameSvgItem {
                        id: mask
                        anchors.fill: parent
                        imagePath: pane.asset; prefix: "blurred-mask"
                        layer.enabled: true; visible: false
                    }
                    MultiEffect {
                        anchors.fill: parent
                        source: wallpaperCrop
                        blurEnabled: true; blur: 1; blurMax: 128
                        autoPaddingEnabled: false
                        maskEnabled: true; maskSource: mask
                    }
                    KSvg.FrameSvgItem {
                        anchors.fill: parent
                        imagePath: pane.asset; prefix: "blurred"
                    }
                    Text {
                        x: 24; y: 22; text: "Desktop widgets"
                        color: pane.ink; font.family: "Nunito"; font.pixelSize: 22; font.weight: Font.DemiBold
                    }
                    Text {
                        x: 24; y: 55; text: "Soft glass. Wallpaper color."
                        color: pane.ink; opacity: 0.75; font.family: "Nunito"; font.pixelSize: 13
                    }
                    Grid {
                        x: 24; y: 96; columns: 3; spacing: 10
                        Repeater {
                            model: 9
                            Rectangle {
                                required property int index
                                width: 85; height: 66; radius: 14
                                color: pane.modelData === "light" ? "#30ffffff" : "#18ffffff"
                                border.color: pane.modelData === "light" ? "#60ffffff" : "#30ffffff"
                                Text { anchors.centerIn: parent; text: index + 1; color: pane.ink; font.family: "Nunito"; font.pixelSize: 20 }
                            }
                        }
                    }
                }
            }
        }
    }
}
