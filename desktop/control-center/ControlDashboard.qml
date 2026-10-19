// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC

PC.ScrollView {
    id: dashboard
    required property var backend
    signal openApplet(string plugin)
    property var customItems: ["org.kde.plasma.networkmanagement", "org.kde.plasma.bluetooth", "org.kde.plasma.notifications", "org.kde.plasma.volume", "org.kde.plasma.brightness", "org.kde.plasma.mediacontroller"]
    property var otherItems: []
    function includes(plugin) { return customItems.includes(plugin); }
    background: null
    contentWidth: availableWidth
    clip: true
    implicitWidth: 289
    implicitHeight: 397
    PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
    PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff

    component Card: Rectangle {
        radius: 15
        color: Kirigami.Theme.backgroundColor.r < 0.5 ? Qt.tint(Kirigami.Theme.backgroundColor, "#0dffffff") : Qt.tint(Kirigami.Theme.backgroundColor, "#88ffffff")
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        Layout.fillWidth: true
    }
    component IconButton: PC.ToolButton {
        id: button
        implicitWidth: 26
        implicitHeight: 26
        padding: 4
        display: PC.AbstractButton.IconOnly
        contentItem: Kirigami.Icon {
            source: button.icon.name
            implicitWidth: 18; implicitHeight: 18
            isMask: true
            color: button.enabled ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
        }
        PC.ToolTip { text: parent.text }
    }
    ColumnLayout {
        objectName: "dashboardContent"
        width: dashboard.availableWidth
        spacing: 7
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 7; columnSpacing: 7
            ControlTile {
                Layout.fillWidth: true
                visible: dashboard.includes("org.kde.plasma.networkmanagement")
                title: dashboard.backend.wifiAvailable ? qsTr("Wi-Fi") : qsTr("Network")
                subtitle: dashboard.backend.networkName
                symbol: dashboard.backend.networkIcon
                selected: dashboard.backend.wifiAvailable ? dashboard.backend.wifiEnabled : false
                available: dashboard.backend.wifiAvailable ? dashboard.backend.wifiToggleAllowed : true
                onTriggered: dashboard.backend.wifiAvailable ? dashboard.backend.toggleWifi() : dashboard.openApplet("org.kde.plasma.networkmanagement")
                onDetailsRequested: dashboard.openApplet("org.kde.plasma.networkmanagement")
            }
            ControlTile {
                Layout.fillWidth: true
                visible: dashboard.includes("org.kde.plasma.bluetooth")
                title: qsTr("Bluetooth")
                subtitle: dashboard.backend.bluetoothAvailable ? dashboard.backend.bluetoothName : qsTr("No adapter")
                symbol: "network-bluetooth-symbolic"
                selected: dashboard.backend.bluetoothEnabled
                available: dashboard.backend.bluetoothAvailable
                onTriggered: dashboard.backend.toggleBluetooth()
                onDetailsRequested: dashboard.openApplet("org.kde.plasma.bluetooth")
            }
            ControlTile {
                Layout.fillWidth: true
                visible: dashboard.includes("org.kde.plasma.notifications")
                title: qsTr("Do Not Disturb")
                subtitle: dashboard.backend.dndEnabled ? qsTr("Notifications paused") : qsTr("Notifications on")
                symbol: dashboard.backend.dndEnabled ? "notifications-disabled-symbolic" : "notifications-symbolic"
                selected: dashboard.backend.dndEnabled
                available: dashboard.backend.dndAvailable
                onTriggered: dashboard.backend.toggleDnd()
                onDetailsRequested: dashboard.openApplet("org.kde.plasma.notifications")
            }
            ControlTile {
                Layout.fillWidth: true
                visible: dashboard.includes("org.kde.plasma.volume")
                title: qsTr("Microphone")
                subtitle: !dashboard.backend.microphoneAvailable ? qsTr("No microphone") : dashboard.backend.microphoneMuted ? qsTr("Muted") : qsTr("Ready")
                symbol: dashboard.backend.microphoneMuted ? "microphone-sensitivity-muted-symbolic" : "audio-input-microphone-symbolic"
                selected: dashboard.backend.microphoneMuted
                available: dashboard.backend.microphoneAvailable
                onTriggered: dashboard.backend.toggleMicrophone()
                onDetailsRequested: dashboard.openApplet("org.kde.plasma.volume")
            }
        }

        Card {
            objectName: "volumeCard"
            visible: dashboard.includes("org.kde.plasma.volume")
            implicitHeight: soundLayout.implicitHeight + 16
            ColumnLayout {
                id: soundLayout
                anchors.fill: parent; anchors.margins: 8
                spacing: 0
                RowLayout {
                    PC.Label { text: qsTr("Volume"); font.weight: Font.DemiBold; Layout.fillWidth: true }
                    PC.Label { text: dashboard.backend.muted ? qsTr("Muted") : dashboard.backend.volume + "%"; color: Kirigami.Theme.disabledTextColor }
                }
                RowLayout {
                    IconButton {
                        objectName: "muteButton"
                        text: dashboard.backend.muted ? qsTr("Unmute speakers") : qsTr("Mute speakers")
                        icon.name: dashboard.backend.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
                        enabled: dashboard.backend.volumeAvailable
                        onClicked: dashboard.backend.toggleMute()
                    }
                    ControlSlider {
                        objectName: "volumeSlider"
                        Layout.fillWidth: true
                        enabled: dashboard.backend.volumeAvailable
                        value: dashboard.backend.volume
                        Accessible.name: qsTr("Volume")
                        onMoved: dashboard.backend.setVolume(value)
                    }
                    IconButton { text: qsTr("Audio devices: %1").arg(dashboard.backend.outputName); icon.name: "go-next-symbolic"; onClicked: dashboard.openApplet("org.kde.plasma.volume") }
                }
            }
        }
        Card {
            objectName: "brightnessCard"
            visible: dashboard.includes("org.kde.plasma.brightness")
            implicitHeight: displayLayout.implicitHeight + 16
            ColumnLayout {
                id: displayLayout
                anchors.fill: parent; anchors.margins: 8
                spacing: 0
                RowLayout {
                    PC.Label { text: qsTr("Display"); font.weight: Font.DemiBold; Layout.fillWidth: true }
                    IconButton { text: qsTr("Brightness and color"); icon.name: "video-display"; onClicked: dashboard.openApplet("org.kde.plasma.brightness") }
                }
                Repeater {
                    model: dashboard.backend.displays
                    delegate: RowLayout {
                        required property string displayName
                        required property string label
                        required property int brightness
                        required property int maxBrightness
                        Layout.fillWidth: true
                        Kirigami.Icon { source: "brightness-high-symbolic"; isMask: true; color: Kirigami.Theme.textColor; implicitWidth: 18; implicitHeight: 18; Layout.leftMargin: 4; Layout.rightMargin: 4 }
                        ControlSlider {
                            Layout.fillWidth: true
                            from: 1; to: Math.max(1, parent.maxBrightness)
                            value: parent.brightness
                            enabled: parent.maxBrightness > 1
                            Accessible.name: qsTr("Brightness: %1").arg(parent.label)
                            onMoved: dashboard.backend.setBrightness(parent.displayName, value)
                        }
                        PC.Label { text: Math.round(100 * parent.brightness / Math.max(1, parent.maxBrightness)) + "%"; Layout.minimumWidth: 34; horizontalAlignment: Text.AlignRight; color: Kirigami.Theme.disabledTextColor }
                    }
                }
                PC.Label {
                    visible: !dashboard.backend.brightnessAvailable
                    text: qsTr("No adjustable display detected")
                    color: Kirigami.Theme.disabledTextColor
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9
                }
            }
        }
        Card {
            objectName: "mediacontrollerCard"
            visible: dashboard.includes("org.kde.plasma.mediacontroller")
            implicitHeight: mediaLayout.implicitHeight + 16
            RowLayout {
                id: mediaLayout
                anchors.fill: parent; anchors.margins: 8
                spacing: 8
                Rectangle {
                    implicitWidth: 42; implicitHeight: 42; radius: 10
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
                    Kirigami.Icon { anchors.centerIn: parent; width: 22; height: 22; source: "media-playback-start-symbolic"; isMask: true; color: Kirigami.Theme.textColor; visible: cover.status !== Image.Ready }
                    Image { id: cover; anchors.fill: parent; source: dashboard.backend.artwork; fillMode: Image.PreserveAspectFit; asynchronous: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 0
                    PC.Label { text: dashboard.backend.track; Layout.fillWidth: true; font.weight: Font.DemiBold; elide: Text.ElideRight; textFormat: Text.PlainText }
                    PC.Label { text: dashboard.backend.artist; Layout.fillWidth: true; elide: Text.ElideRight; textFormat: Text.PlainText; color: Kirigami.Theme.disabledTextColor; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.9 }
                    RowLayout {
                        spacing: 2
                        IconButton { text: qsTr("Previous track"); icon.name: "media-skip-backward-symbolic"; enabled: dashboard.backend.canPrevious; onClicked: dashboard.backend.previous() }
                        IconButton { objectName: "playPauseButton"; text: dashboard.backend.playing ? qsTr("Pause") : qsTr("Play"); icon.name: dashboard.backend.playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic"; enabled: dashboard.backend.canPlayPause; onClicked: dashboard.backend.playPause() }
                        IconButton { text: qsTr("Next track"); icon.name: "media-skip-forward-symbolic"; enabled: dashboard.backend.canNext; onClicked: dashboard.backend.next() }
                        Item { Layout.fillWidth: true }
                        IconButton { text: qsTr("Media players"); icon.name: "go-next-symbolic"; onClicked: dashboard.openApplet("org.kde.plasma.mediacontroller") }
                    }
                }
            }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 7; columnSpacing: 7
            visible: dashboard.otherItems.length > 0
            Repeater {
                model: dashboard.otherItems
                delegate: ControlTrayEntry {
                    required property var modelData
                    Layout.fillWidth: true
                    entry: modelData
                    onOpenApplet: plugin => dashboard.openApplet(plugin)
                }
            }
        }
        PC.Label {
            visible: dashboard.customItems.length === 0 && dashboard.otherItems.length === 0
            Layout.fillWidth: true
            text: qsTr("No items to show. Choose ‘Show when relevant’ or ‘Show only in popup’ in System Tray settings.")
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.disabledTextColor
        }
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            IconButton { text: qsTr("System Settings"); icon.name: "configure"; onClicked: dashboard.backend.settings() }
            IconButton { text: qsTr("Lock screen"); icon.name: "system-lock-screen-symbolic"; enabled: dashboard.backend.canLock; onClicked: dashboard.backend.lock() }
            IconButton { text: qsTr("Power and session"); icon.name: "system-shutdown-symbolic"; onClicked: dashboard.backend.powerOptions() }
        }
    }
}
