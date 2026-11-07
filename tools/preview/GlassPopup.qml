// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

PlasmaCore.Dialog {
    x: 428; y: 270
    visible: true
    location: PlasmaCore.Types.Floating
    hideOnWindowDeactivate: false
    mainItem: Item {
        width: 320; height: 210
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 16
            RowLayout {
                spacing: 12
                Rectangle {
                    width: 58; height: 58; radius: 14
                    gradient: Gradient {
                        GradientStop { position: 0; color: "#50bce4" }
                        GradientStop { position: 1; color: "#6841d8" }
                    }
                    Kirigami.Icon { anchors.centerIn: parent; width: 30; height: 30; source: "audio-x-generic" }
                }
                ColumnLayout {
                    spacing: 4
                    Text { text: "Now Playing"; color: Kirigami.Theme.textColor; font.pixelSize: 18; font.bold: true }
                    Text { text: "Glass material preview"; color: Kirigami.Theme.textColor; opacity: 0.8; font.pixelSize: 13 }
                }
            }
            Item { Layout.fillHeight: true }
            Rectangle {
                Layout.fillWidth: true; height: 4; radius: 2; color: Qt.alpha(Kirigami.Theme.textColor,0.22)
                Rectangle { width: parent.width*0.38; height: 4; radius: 2; color: Kirigami.Theme.textColor }
            }
            RowLayout {
                Layout.fillWidth: true
                Repeater {
                    model: ["media-playlist-repeat", "media-skip-backward", "media-playback-pause", "media-skip-forward", "audio-volume-high"]
                    Item {
                        required property string modelData
                        Layout.fillWidth: true; height: 32
                        Kirigami.Icon { anchors.centerIn: parent; width: 22; height: 22; source: modelData }
                    }
                }
            }
        }
    }
}
