// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.private.mpris as Mpris

Item {
    property var appearance: null
    id: mediaControls
    visible: players.count > 0
    implicitHeight: visible ? 156 : 0
    implicitWidth: 392
    Repeater {
        id: players
        model: Mpris.MultiplexerModel { }
        SequoiaMediaCard {
            required property var model
            appearance: mediaControls.appearance
            anchors.fill: parent
            trackTitle: model.track
            artist: model.artist
            playerName: model.identity
            artwork: model.artUrl
            playing: model.playbackStatus === Mpris.PlaybackStatus.Playing
            canPlay: model.canControl && model.canPlay
            canPause: model.canControl && model.canPause
            canPrevious: model.canControl && model.canGoPrevious
            canNext: model.canControl && model.canGoNext
            onPrevious: { fadeoutTimer.running = false; model.container.Previous(); }
            onPlayPause: {
                fadeoutTimer.running = false;
                if (playing) model.container.Pause();
                else model.container.Play();
            }
            onNext: { fadeoutTimer.running = false; model.container.Next(); }
        }
    }
}
