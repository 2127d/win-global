// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

Rectangle {
    id: card
    objectName: "sequoiaMediaCard"
    property string trackTitle: ""
    property string artist: ""
    property string playerName: ""
    property url artwork
    property bool playing: false
    property bool canPlay: true
    property bool canPause: true
    property bool canPrevious: true
    property bool canNext: true
    signal previous()
    signal playPause()
    signal next()
    property var appearance: null
    // Standalone media previews fall back to the desktop palette.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Binding {
        target: card.Kirigami.Theme
        property: "textColor"
        value: card.appearance ? card.appearance.foreground : "white"
        when: card.appearance !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
    readonly property color foreground: appearance ? appearance.foreground : Kirigami.Theme.textColor
    readonly property color mutedForeground: appearance ? appearance.muted : Kirigami.Theme.disabledTextColor
    implicitWidth: 392
    implicitHeight: 156
    radius: 20
    color: appearance ? appearance.surface : Qt.alpha(Kirigami.Theme.backgroundColor, 0.82)
    border.color: Qt.alpha(foreground, 0.16)
    border.width: 1

    RowLayout {
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
        spacing: 14
        Rectangle {
            id: artFrame
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            radius: 12
            color: Qt.alpha(card.foreground, 0.08)
            Kirigami.Icon {
                anchors.centerIn: parent
                width: 30; height: 30
                source: "audio-x-generic"
                visible: art.status !== Image.Ready
                color: card.foreground
                active: false
            }
            Image {
                id: art
                anchors.fill: parent
                source: card.artwork
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                sourceSize: Qt.size(128, 128)
                layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                layer.effect: OpacityMask {
                    maskSource: Rectangle { width: artFrame.width; height: artFrame.height; radius: artFrame.radius }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: card.playing ? qsTr("NOW PLAYING") : qsTr("PAUSED")
                color: card.mutedForeground
                font.pixelSize: 9
                font.letterSpacing: 1.7
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: card.trackTitle || qsTr("Untitled track")
                color: card.foreground
                font.pixelSize: 15
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
            Text {
                Layout.fillWidth: true
                text: card.artist || card.playerName
                color: card.mutedForeground
                font.pixelSize: 12
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
        }
    }
    Rectangle {
        x: 16; y: 94; width: parent.width - 32; height: 1
        color: Qt.alpha(card.foreground, 0.10)
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 104
        spacing: 14
        PC.ToolButton {
            width: 36; height: 36
            enabled: card.canPrevious
            icon.name: "media-skip-backward"
            icon.width: 20; icon.height: 20
            hoverEnabled: false
            Accessible.name: qsTr("Previous track")
            onClicked: card.previous()
        }
        PC.ToolButton {
            id: play
            width: 48; height: 36
            enabled: card.playing ? card.canPause : card.canPlay
            hoverEnabled: false
            icon.name: card.playing ? "media-playback-pause" : "media-playback-start"
            icon.width: 22; icon.height: 22
            Accessible.name: card.playing ? qsTr("Pause") : qsTr("Play")
            background: Rectangle {
                radius: 12
                color: Qt.alpha(card.foreground, play.down ? 0.20 : 0.09)
                border.color: play.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(card.foreground, 0.16)
            }
            onClicked: card.playPause()
        }
        PC.ToolButton {
            width: 36; height: 36
            enabled: card.canNext
            icon.name: "media-skip-forward"
            icon.width: 20; icon.height: 20
            hoverEnabled: false
            Accessible.name: qsTr("Next track")
            onClicked: card.next()
        }
    }
}
