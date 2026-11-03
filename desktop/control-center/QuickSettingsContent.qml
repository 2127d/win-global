// SPDX-License-Identifier: MIT
import QtQuick
import "SequoiaPopup" as Popup
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

Loader {
    id: content
    required property var controller
    required property var spec
    sourceComponent: spec.kind === "display" ? displayComponent
        : spec.kind === "volume" ? volumeComponent : spec.kind === "media" ? mediaComponent
        : spec.kind === "tray" ? trayComponent : toggleComponent

    component IconButton: Popup.GlassToolButton {
        id: button
        implicitWidth: 26; implicitHeight: 26; padding: 4
        display: PC.AbstractButton.IconOnly
        contentItem: Kirigami.Icon {
            source: button.icon.name
            implicitWidth: 18; implicitHeight: 18; isMask: true
            color: button.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
        }
        PC.ToolTip { text: parent.text }
    }
    Component {
        id: toggleComponent
        ControlTile {
            title: content.spec.title
            subtitle: content.controller.subtitle(content.spec.id)
            symbol: content.controller.symbol(content.spec.id)
            selected: content.controller.isSelected(content.spec.id)
            available: content.controller.isAvailable(content.spec.id)
            checkable: content.spec.kind === "toggle"
            onTriggered: content.controller.activate(content.spec.id)
            onDetailsRequested: content.controller.openApplet(content.spec.plugin)
        }
    }
    Component {
        id: trayComponent
        ControlTrayEntry {
            entry: content.spec.entry
            onOpenApplet: plugin => content.controller.openApplet(plugin)
        }
    }
    Component {
        id: displayComponent
ControlGlassSurface {
    id: displayCard
    objectName: "brightnessCard"
    radius: 24
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 12; anchors.bottomMargin: 12
                spacing: 4
                Item { Layout.fillHeight: true }
                RowLayout {
                    PC.Label { text: qsTr("Display"); font.weight: Font.DemiBold; Layout.fillWidth: true }
                    IconButton { objectName: "displayDetails"; text: qsTr("Brightness and color"); icon.name: "video-display"; onClicked: content.controller.openApplet("org.kde.plasma.brightness") }
                }
                PC.ScrollView {
                    id: displayScroll
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(displayRows.implicitHeight, Math.max(30, displayCard.height - 64))
                    Layout.minimumHeight: 30
                    contentWidth: availableWidth
                    clip: true
                    background: null
                    PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
                    PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
                    ColumnLayout {
                        id: displayRows
                        width: displayScroll.availableWidth
                        spacing: 4
                        Repeater {
                            model: content.controller.backend.displays
                            delegate: RowLayout {
                                required property string displayName
                                required property string label
                                required property int brightness
                                required property int maxBrightness
                                Layout.fillWidth: true
                                spacing: 6
                                Kirigami.Icon { source: "brightness-high-symbolic"; isMask: true; color: Kirigami.Theme.textColor; implicitWidth: 18; implicitHeight: 18 }
                                ControlSlider {
                                    objectName: "brightnessSlider:" + parent.displayName
                                    Layout.fillWidth: true
                                    from: 1; to: Math.max(1, parent.maxBrightness)
                                    value: parent.brightness
                                    enabled: parent.maxBrightness > 1
                                    Accessible.name: qsTr("Brightness: %1").arg(parent.label)
                                    onMoved: content.controller.backend.setBrightness(parent.displayName, value)
                                    PC.ToolTip { text: parent.Accessible.name }
                                }
                                PC.Label { text: Math.round(100 * parent.brightness / Math.max(1, parent.maxBrightness)) + "%"; Layout.minimumWidth: 34; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.disabledTextColor }
                            }
                        }
                        PC.Label {
                            visible: !content.controller.backend.brightnessAvailable
                            text: qsTr("No adjustable display detected")
                            color: Kirigami.Theme.disabledTextColor
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9
                        }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }
    Component {
        id: mediaComponent
ControlGlassSurface {
    id: card
    objectName: "mediacontrollerCard"
    radius: 24
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10
                    spacing: 2
                    RowLayout {
                        Rectangle {
                            implicitWidth: 28; implicitHeight: 28; radius: 10
                            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
                            Kirigami.Icon { anchors.centerIn: parent; width: 20; height: 20; source: "media-playback-start-symbolic"; isMask: true; color: Kirigami.Theme.textColor; visible: cover.status !== Image.Ready }
                            Image { id: cover; anchors.fill: parent; source: content.controller.backend.artwork && content.controller.backend.artwork !== "null" ? content.controller.backend.artwork : ""; fillMode: Image.PreserveAspectFit; asynchronous: true }
                        }
                        Item { Layout.fillWidth: true }
                        IconButton { objectName: "mediaDetails"; text: qsTr("Media players"); icon.name: "go-next-symbolic"; onClicked: content.controller.openApplet("org.kde.plasma.mediacontroller") }
                    }
                    Item { Layout.fillHeight: true }
                    PC.Label { text: content.controller.backend.track; Layout.fillWidth: true; font.weight: Font.DemiBold; elide: Text.ElideRight; textFormat: Text.PlainText }
                    PC.Label { text: content.controller.backend.artist; Layout.fillWidth: true; elide: Text.ElideRight; textFormat: Text.PlainText; color: Kirigami.Theme.disabledTextColor; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9 }
                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        IconButton { text: qsTr("Previous track"); icon.name: "media-skip-backward-symbolic"; enabled: content.controller.backend.canPrevious; onClicked: content.controller.backend.previous() }
                        Item { Layout.fillWidth: true }
                        IconButton { objectName: "playPauseButton"; text: content.controller.backend.playing ? qsTr("Pause") : qsTr("Play"); icon.name: content.controller.backend.playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"; enabled: content.controller.backend.canPlayPause; onClicked: content.controller.backend.playPause() }
                        Item { Layout.fillWidth: true }
                        IconButton { text: qsTr("Next track"); icon.name: "media-skip-forward-symbolic"; enabled: content.controller.backend.canNext; onClicked: content.controller.backend.next() }
                    }
                }
            }
    }
    Component {
        id: volumeComponent
ControlGlassSurface {
    id: card
    objectName: "volumeCard"
    radius: height / 2
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14; anchors.rightMargin: 14
                        anchors.topMargin: 4; anchors.bottomMargin: 4
                        spacing: 0
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            PC.Label { text: qsTr("Sound"); font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                            PC.Label { text: content.controller.backend.muted ? qsTr("Muted") : content.controller.backend.volume + "%"; color: Kirigami.Theme.disabledTextColor; elide: Text.ElideRight }
                        }
                        RowLayout {
                            spacing: 4
                            IconButton {
                                objectName: "muteButton"
                                text: content.controller.backend.muted ? qsTr("Unmute speakers") : qsTr("Mute speakers")
                                icon.name: content.controller.backend.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
                                enabled: content.controller.backend.volumeAvailable
                                onClicked: content.controller.backend.toggleMute()
                            }
                            ControlSlider {
                                objectName: "volumeSlider"
                                implicitHeight: 30
                                Layout.fillWidth: true
                                enabled: content.controller.backend.volumeAvailable
                                value: content.controller.backend.volume
                                Accessible.name: qsTr("Volume")
                                onMoved: content.controller.backend.setVolume(value)
                            }
                        }
                    }
                }
    }
}
