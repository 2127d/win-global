// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.private.mpris as Mpris

FocusScope {
    id: media
    objectName: "sequoiaMediaPlayer"
    required property var players
    readonly property var player: players.currentPlayer
    readonly property bool playing: player?.playbackStatus === Mpris.PlaybackStatus.Playing
    readonly property real duration: Math.max(0, (player?.length ?? 0) / 1000000)
    property real displayedPosition: 0
    readonly property color surface: Qt.tint(Kirigami.Theme.backgroundColor,
        Kirigami.Theme.backgroundColor.r < 0.5 ? "#12ffffff" : "#70ffffff")

    function timestamp(seconds) {
        const s = Math.max(0, Math.floor(seconds));
        return (s >= 3600 ? Math.floor(s / 3600) + ":" : "")
            + (s >= 3600 ? String(Math.floor(s / 60) % 60).padStart(2, "0") : Math.floor(s / 60))
            + ":" + String(s % 60).padStart(2, "0");
    }
    function syncPosition() {
        if (!seek.pressed && !seekUpdate.running) displayedPosition = (player?.position ?? 0) / 1000000;
    }
    function resetPosition() {
        seekUpdate.stop();
        displayedPosition = (player?.position ?? 0) / 1000000;
        if (visible) player?.updatePosition();
    }
    function togglePlaying() {
        if (playing && player.canPause) player.Pause();
        else if (player?.canPlay) player.Play();
    }
    function seekBy(seconds) {
        if (!player?.canSeek || duration <= 0) return;
        displayedPosition = Math.max(0, Math.min(duration, displayedPosition + seconds));
        seekUpdate.restart();
    }
    onPlayerChanged: resetPosition()
    onDurationChanged: resetPosition()
    onVisibleChanged: if (visible) resetPosition()
    Connections {
        target: media.player
        function onPositionChanged() { media.syncPosition(); }
        function onTrackChanged() { media.resetPosition(); }
    }
    Timer {
        id: seekUpdate
        interval: 100
        onTriggered: if (media.player?.canSeek) media.player.position = Math.round(media.displayedPosition * 1000000)
    }
    Timer {
        interval: 1000; repeat: true
        running: media.visible && media.playing && media.duration > 0 && !seek.pressed && !seekUpdate.running
        onTriggered: media.displayedPosition = Math.min(media.duration, media.displayedPosition + (media.player?.rate ?? 1))
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_K) { togglePlaying(); event.accepted = true; }
        else if (event.key === Qt.Key_J) { seekBy(-10); event.accepted = true; }
        else if (event.key === Qt.Key_L) { seekBy(10); event.accepted = true; }
        else if ((event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) && event.modifiers & Qt.ControlModifier && tabs.count) {
            const delta = event.key === Qt.Key_Backtab || event.modifiers & Qt.ShiftModifier ? -1 : 1;
            players.currentIndex = (players.currentIndex + delta + tabs.count) % tabs.count;
            event.accepted = true;
        }
    }

    component RoundButton: PC.ToolButton {
        id: button
        property bool prominent: false
        implicitWidth: prominent ? 54 : 38
        implicitHeight: implicitWidth
        padding: prominent ? 14 : 10
        display: PC.AbstractButton.IconOnly
        opacity: enabled ? 1 : 0.3
        contentItem: Kirigami.Icon {
            source: button.icon.name
            isMask: true
            color: button.prominent ? Kirigami.Theme.highlightedTextColor : button.checked ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
        }
        background: Rectangle {
            radius: width / 2
            color: button.prominent ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, button.down ? 0.13 : button.hovered ? 0.08 : button.checked ? 0.05 : 0)
            border.width: button.activeFocus ? 2 : 0
            border.color: Kirigami.Theme.highlightColor
            scale: button.down ? 0.94 : 1
            Behavior on scale { NumberAnimation { duration: 100 } }
        }
        PC.ToolTip { text: button.text }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10
        ListView {
            id: tabs
            objectName: "mediaPlayerTabs"
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            visible: count > 0
            orientation: ListView.Horizontal
            spacing: 6; clip: true
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: media.players.currentIndex
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
            model: media.players
            delegate: PC.AbstractButton {
                id: tab
                required property int index
                required property string identity
                required property string iconName
                required property bool isMultiplexer
                objectName: "mediaPlayerTab:" + identity
                width: Math.min(180, tabContent.implicitWidth + 24)
                height: 36
                leftPadding: 12; rightPadding: 12
                text: isMultiplexer ? qsTr("Auto") : identity
                checked: index === media.players.currentIndex
                Accessible.role: Accessible.PageTab
                Accessible.name: isMultiplexer ? qsTr("Choose player automatically") : identity
                onClicked: media.players.currentIndex = index
                Keys.onRightPressed: media.players.currentIndex = Math.min(tabs.count - 1, index + 1)
                Keys.onLeftPressed: media.players.currentIndex = Math.max(0, index - 1)
                background: Rectangle {
                    radius: 18
                    color: tab.checked ? Kirigami.Theme.highlightColor : media.surface
                    border.width: tab.activeFocus ? 2 : 1
                    border.color: Qt.alpha(tab.checked ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor, tab.activeFocus ? 0.8 : 0.07)
                }
                contentItem: RowLayout {
                    id: tabContent
                    spacing: 7
                    Kirigami.Icon {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        source: tab.isMultiplexer ? "media-playback-start-symbolic" : tab.iconName
                        isMask: true
                        color: tab.checked ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                    PC.Label {
                        text: tab.text
                        color: tab.checked ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                        font.weight: tab.checked ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                PC.ToolTip { text: tab.Accessible.name }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 72
            radius: 24
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.tint(media.surface, Qt.alpha(Kirigami.Theme.highlightColor, 0.12)) }
                GradientStop { position: 1; color: media.surface }
            }
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
            MediaArtwork {
                anchors.fill: parent
                anchors.margins: 10
                source: media.player?.artUrl ?? ""
                fallbackIcon: media.player?.iconName || "media-album-cover"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                PC.Label {
                    objectName: "mediaTrackTitle"
                    Layout.fillWidth: true
                    text: media.player?.track || (media.player ? qsTr("Untitled media") : qsTr("Nothing playing yet"))
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
                    font.weight: Font.DemiBold
                    maximumLineCount: 2; wrapMode: Text.Wrap; elide: Text.ElideRight
                    textFormat: Text.PlainText
                }
                PC.Label {
                    Layout.fillWidth: true
                    text: media.player?.artist || media.player?.identity || qsTr("Start audio or video in an app")
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight; textFormat: Text.PlainText
                }
            }
            RoundButton {
                text: qsTr("Open %1").arg(media.player?.identity ?? qsTr("player"))
                icon.name: "window-new-symbolic"
                visible: media.player?.canRaise ?? false
                onClicked: media.player.Raise()
            }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: transport.implicitHeight + 20
            radius: 22
            color: media.surface
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.07)
            ColumnLayout {
                id: transport
                anchors.fill: parent; anchors.margins: 10
                spacing: 3
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    visible: media.duration > 0
                    PC.Slider {
                        id: seek
                        objectName: "mediaSeekSlider"
                        Layout.fillWidth: true
                        implicitHeight: 22
                        from: 0; to: Math.max(1, media.duration)
                        value: media.displayedPosition
                        stepSize: 1
                        enabled: media.player?.canSeek ?? false
                        Accessible.name: qsTr("Playback position")
                        onMoved: { media.displayedPosition = value; seekUpdate.restart(); }
                        leftPadding: 0; rightPadding: 0
                        background: Rectangle {
                            x: seek.leftPadding; y: (seek.height - height) / 2
                            width: seek.availableWidth; height: 5; radius: 2.5
                            color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
                            Rectangle {
                                x: seek.mirrored ? parent.width - width : 0
                                width: seek.position * parent.width; height: parent.height; radius: 2.5
                                color: Kirigami.Theme.highlightColor
                            }
                        }
                        handle: Rectangle {
                            x: seek.leftPadding + seek.visualPosition * (seek.availableWidth - width)
                            y: (seek.height - height) / 2
                            width: seek.pressed ? 14 : 11; height: width; radius: width / 2
                            visible: seek.enabled
                            color: Kirigami.Theme.highlightColor
                            border.width: seek.activeFocus ? 2 : 0
                            border.color: Kirigami.Theme.textColor
                        }
                    }
                    RowLayout {
                        PC.Label { text: media.timestamp(media.displayedPosition); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
                        Item { Layout.fillWidth: true }
                        PC.Label { text: "−" + media.timestamp(media.duration - media.displayedPosition); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor }
                    }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10
                    RoundButton {
                        objectName: "mediaShuffle"
                        text: qsTr("Shuffle")
                        icon.name: "media-playlist-shuffle"
                        enabled: !!media.player?.canControl && media.player.shuffle !== Mpris.ShuffleStatus.Unknown
                        checked: media.player?.shuffle === Mpris.ShuffleStatus.On
                        onClicked: media.player.shuffle = checked ? Mpris.ShuffleStatus.Off : Mpris.ShuffleStatus.On
                    }
                    RoundButton {
                        objectName: "mediaPrevious"
                        text: qsTr("Previous track"); icon.name: "media-skip-backward"
                        enabled: media.player?.canGoPrevious ?? false
                        onClicked: media.player.Previous()
                    }
                    RoundButton {
                        objectName: "mediaPlayPause"
                        prominent: true
                        text: media.playing ? qsTr("Pause") : qsTr("Play")
                        icon.name: media.playing ? "media-playback-pause" : "media-playback-start"
                        enabled: (media.playing ? media.player?.canPause : media.player?.canPlay) ?? false
                        onClicked: media.togglePlaying()
                    }
                    RoundButton {
                        objectName: "mediaNext"
                        text: qsTr("Next track"); icon.name: "media-skip-forward"
                        enabled: media.player?.canGoNext ?? false
                        onClicked: media.player.Next()
                    }
                    RoundButton {
                        objectName: "mediaRepeat"
                        text: media.player?.loopStatus === Mpris.LoopStatus.Track ? qsTr("Repeat track") : qsTr("Repeat playlist")
                        icon.name: media.player?.loopStatus === Mpris.LoopStatus.Track ? "media-playlist-repeat-song" : "media-playlist-repeat"
                        enabled: !!media.player?.canControl && media.player.loopStatus !== Mpris.LoopStatus.Unknown
                        checked: !!media.player && media.player.loopStatus !== Mpris.LoopStatus.Unknown && media.player.loopStatus !== Mpris.LoopStatus.None
                        onClicked: media.player.loopStatus = media.player.loopStatus === Mpris.LoopStatus.Playlist ? Mpris.LoopStatus.Track
                            : media.player.loopStatus === Mpris.LoopStatus.Track ? Mpris.LoopStatus.None : Mpris.LoopStatus.Playlist
                    }
                }
            }
        }
    }
}
