// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.sequoia.login 1.0

Item {
    id: island
    required property Item scene
    required property var status
    readonly property real unit: Math.max(0.8, scene.loginUnit * 0.85)
    property string section: "clock"
    property alias popup: details
    property var sessionModel: null
    property int sessionIndex: 0
    property string sessionDescription: qsTr("Desktop session")
    signal sessionSelected(int index)
    signal dismissed()
    implicitWidth: indicators.implicitWidth + 12 * unit
    implicitHeight: 38 * unit
    visible: status !== null

    function showSection(name) {
        if (details.opened && section === name) { details.close(); return; }
        if (details.visible && section !== name) details.close();
        section = name;
        details.open();
    }
    onEnabledChanged: { if (!enabled) details.close(); }

    WallpaperGlass {
        anchors.fill: parent
        source: island.scene.wallpaper
        sceneSize: Qt.size(island.scene.width, island.scene.height)
        sampleRect: Qt.rect(island.x, island.y, width, height)
        blurRadius: 24 * island.scene.unit
        radius: 13 * island.unit
        tint: "#65121d22"
        Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.color: "#407b929c" }
    }
    component StatusAction: Controls.AbstractButton {
        id: action
        property string symbol: ""
        property string label: ""
        property bool trailingSymbol: false
        property bool selected: false
        implicitWidth: contents.implicitWidth + 16 * island.unit
        implicitHeight: 32 * island.unit
        padding: 0
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.name: text
        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: 600
        background: Rectangle {
            radius: 9 * island.unit
            color: action.down || action.selected ? "#354dabdd" : action.hovered ? "#20ffffff" : "transparent"
            border.width: action.visualFocus ? 1.5 : 0
            border.color: "#a9d5ed"
        }
        contentItem: Item {
            Row {
                id: contents
                objectName: action.objectName + "Contents"
                spacing: 6 * island.unit
                layoutDirection: action.trailingSymbol ? Qt.RightToLeft : Qt.LeftToRight
                anchors.centerIn: parent
                StatusGlyph {
                    visible: action.symbol.length > 0
                    symbol: action.symbol
                    batteryPercent: island.status ? island.status.batteryPercent : 0
                    width: 16 * island.unit; height: width
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    visible: action.label.length > 0
                    text: action.label
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"; font.family: "Adwaita Sans"; font.pixelSize: 14 * island.unit
                }
            }
        }
    }
    Row {
        id: indicators
        anchors.centerIn: parent
        spacing: 2 * island.unit
        StatusAction {
            id: sessionButton
            objectName: "sessionStatusButton"
            visible: sessionChoices.count > 1
            symbol: "desktop-session"
            text: island.sessionDescription
            selected: details.opened && island.section === "session"
            onClicked: island.showSection("session")
        }
        StatusAction {
            objectName: "networkStatusButton"
            symbol: island.status ? island.status.networkIcon : "network-offline-symbolic"
            text: qsTr("Network")
            selected: details.opened && island.section === "network"
            onClicked: island.showSection("network")
        }
        StatusAction {
            objectName: "audioStatusButton"
            symbol: !island.status || !island.status.audioAvailable || island.status.muted || island.status.volumePercent === 0 ? "audio-volume-muted-symbolic" :
                island.status.volumePercent < 34 ? "audio-volume-low-symbolic" : island.status.volumePercent < 67 ? "audio-volume-medium-symbolic" : "audio-volume-high-symbolic"
            text: qsTr("Volume")
            selected: details.opened && island.section === "audio"
            onClicked: island.showSection("audio")
        }
        StatusAction {
            objectName: "batteryStatusButton"
            trailingSymbol: true
            visible: island.status ? island.status.hasBattery : false
            symbol: island.status ? island.status.batteryIcon : ""
            label: island.status ? qsTr("%1%").arg(island.status.batteryPercent) : ""
            text: qsTr("Battery")
            selected: details.opened && island.section === "battery"
            onClicked: island.showSection("battery")
        }
        Rectangle { width: 1; height: 16 * island.unit; anchors.verticalCenter: parent.verticalCenter; color: "#307b929c" }
        StatusAction {
            objectName: "clockStatusButton"
            label: island.scene.clockText
            text: island.scene.dateText
            selected: details.opened && island.section === "clock"
            onClicked: island.showSection("clock")
        }
    }
    component DetailText: Text {
        width: parent.width
        color: "#e3ebef"
        font.family: "Adwaita Sans"
        font.pixelSize: 13 * island.unit
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
    }
    Controls.Popup {
        id: details
        objectName: "statusPopup"
        popupType: Controls.Popup.Item
        property real slideOffset: 0
        x: island.section === "session" ? Math.min(indicators.x + sessionButton.x, island.width - width) : island.width - width
        y: island.height + 10 * island.unit + slideOffset
        width: Math.min((island.section === "session" ? 240 : 292) * island.unit, island.scene.width - 36 * island.scene.unit)
        padding: 18 * island.unit
        focus: true
        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
        onClosed: island.dismissed()
        onOpened: {
            if (island.section === "session" && sessionChoices.count > 0)
                sessionChoices.itemAt(Math.max(0, Math.min(island.sessionIndex, sessionChoices.count - 1))).forceActiveFocus();
        }
        enter: Transition {
            ParallelAnimation {
                NumberAnimation { target: details; property: "slideOffset"; from: -12 * island.unit; to: 0; duration: island.section === "session" ? 160 : 0; easing.type: Easing.OutCubic }
                NumberAnimation { target: details; property: "opacity"; from: 0; to: 1; duration: island.section === "session" ? 140 : 0 }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { target: details; property: "slideOffset"; to: -6 * island.unit; duration: island.section === "session" ? 100 : 0; easing.type: Easing.InCubic }
                NumberAnimation { target: details; property: "opacity"; to: 0; duration: island.section === "session" ? 100 : 0 }
            }
        }
        background: WallpaperGlass {
            source: island.scene.wallpaper
            sceneSize: Qt.size(island.scene.width, island.scene.height)
            sampleRect: Qt.rect(island.x + details.x, island.y + details.y, width, height)
            blurRadius: 24 * island.scene.unit
            radius: 16 * island.unit
            tint: "#bb121d22"
            Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.color: "#607b929c" }
        }
        contentItem: Column {
            spacing: 12 * island.unit
            RowLayout {
                width: parent.width
                Text {
                    Layout.fillWidth: true
                    text: island.section === "session" ? qsTr("Desktop session") : island.section === "network" ? qsTr("Network") : island.section === "audio" ? qsTr("Sound") : island.section === "battery" ? qsTr("Battery") : qsTr("Date & time")
                    color: "white"; font.family: "Adwaita Sans"; font.pixelSize: 16 * island.unit; font.weight: Font.DemiBold
                }
                StatusAction { objectName: "statusCloseButton"; symbol: "window-close-symbolic"; text: qsTr("Close"); onClicked: details.close() }
            }
            Controls.ScrollView {
                id: sessionScroll
                width: parent.width
                height: Math.min(sessionColumn.implicitHeight, Math.max(60 * island.unit,
                    island.scene.height - island.y - details.y - 110 * island.unit))
                visible: island.section === "session"
                clip: true
                contentWidth: availableWidth
                Column {
                    id: sessionColumn
                    width: sessionScroll.availableWidth
                    spacing: 4 * island.unit
                    Repeater {
                        id: sessionChoices
                        model: island.sessionModel
                        delegate: Controls.ItemDelegate {
                            id: choice
                            required property int index
                            required property var model
                            objectName: "sessionChoice" + index
                            width: sessionColumn.width
                            height: 36 * island.unit
                            padding: 10 * island.unit
                            text: model.display
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            Accessible.role: Accessible.RadioButton
                            Accessible.name: text
                            Accessible.checkable: true
                            Accessible.checked: island.sessionIndex === index
                            Controls.ToolTip.text: text
                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: 600
                            background: Rectangle {
                                radius: 8 * island.unit
                                color: choice.down || island.sessionIndex === choice.index ? "#354dabdd" : choice.hovered ? "#20ffffff" : "transparent"
                                border.width: choice.visualFocus ? 1.5 : 0
                                border.color: "#a9d5ed"
                            }
                            contentItem: Item {
                                Text {
                                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width - 22 * island.unit
                                    text: choice.text; textFormat: Text.PlainText; elide: Text.ElideRight
                                    color: "white"; font.family: "Adwaita Sans"; font.pixelSize: 13 * island.unit
                                }
                                Image {
                                    visible: island.sessionIndex === choice.index
                                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                    width: 14 * island.unit; height: width
                                    source: "../assets/status-check.svg"; sourceSize.width: width; sourceSize.height: height
                                }
                            }
                            onClicked: { island.sessionSelected(index); details.close(); }
                            Keys.onDownPressed: sessionChoices.itemAt((index + 1) % sessionChoices.count).forceActiveFocus()
                            Keys.onUpPressed: sessionChoices.itemAt((index + sessionChoices.count - 1) % sessionChoices.count).forceActiveFocus()
                        }
                    }
                }
            }
            Column {
                width: parent.width; spacing: 12 * island.unit
                visible: island.section === "network"
                DetailText { text: island.status ? island.status.networkSummary : "" }
                DetailText { text: island.status ? island.status.wifiHint : ""; color: "#afc0cb" }
                StatusAction {
                    objectName: "wifiToggle"
                    visible: island.status ? island.status.wifiAvailable : false
                    enabled: island.status ? island.status.wifiCanChange && !island.status.wifiPending : false
                    opacity: enabled ? 1 : 0.5
                    label: !island.status ? "" : island.status.wifiPending ? qsTr("Updating…") : island.status.wifiEnabled ? qsTr("Turn Wi-Fi off") : qsTr("Turn Wi-Fi on")
                    text: label; symbol: "network-wireless-symbolic"
                    onClicked: island.status.setWifiEnabled(!island.status.wifiEnabled)
                }
                DetailText { text: island.status ? island.status.networkError : ""; visible: text.length > 0; color: "#ffcfad"; Accessible.role: Accessible.AlertMessage }
            }
            Column {
                width: parent.width; spacing: 12 * island.unit
                visible: island.section === "audio"
                DetailText { text: island.status ? island.status.audioDescription : "" }
                RowLayout {
                    width: parent.width
                    StatusAction {
                        objectName: "muteToggle"
                        enabled: island.status ? island.status.audioAvailable : false
                        opacity: enabled ? 1 : 0.5
                        symbol: island.status && island.status.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
                        text: island.status && island.status.muted ? qsTr("Unmute") : qsTr("Mute")
                        onClicked: island.status.toggleMute()
                    }
                    Controls.Slider {
                        id: volume
                        objectName: "statusVolumeSlider"
                        Layout.fillWidth: true
                        enabled: island.status ? island.status.audioAvailable : false
                        from: 0; to: 100; stepSize: 1
                        value: island.status ? island.status.volumePercent : 0
                        Accessible.name: qsTr("Volume")
                        onMoved: island.status.setVolume(value)
                        background: Rectangle {
                            x: volume.leftPadding; y: volume.topPadding + volume.availableHeight / 2 - height / 2
                            implicitHeight: 4 * island.unit; width: volume.availableWidth; height: implicitHeight; radius: height / 2; color: "#506f858c"
                            Rectangle { width: volume.visualPosition * parent.width; height: parent.height; radius: parent.radius; color: "#2da1f6" }
                        }
                        handle: Rectangle {
                            x: volume.leftPadding + volume.visualPosition * (volume.availableWidth - width)
                            y: volume.topPadding + volume.availableHeight / 2 - height / 2
                            width: 16 * island.unit; height: width; radius: width / 2; color: "#f4f8fa"
                            border.color: volume.visualFocus ? "#2da1f6" : "#a7c2c5"; border.width: volume.visualFocus ? 2 : 1
                        }
                    }
                    Text { text: island.status ? qsTr("%1%").arg(island.status.volumePercent) : ""; color: "white"; font.pixelSize: 12 * island.unit }
                }
                DetailText { text: island.status ? island.status.audioError : ""; visible: text.length > 0; color: "#ffcfad"; Accessible.role: Accessible.AlertMessage }
            }
            Column {
                width: parent.width; spacing: 8 * island.unit
                visible: island.section === "battery"
                DetailText { text: island.status && island.status.hasBattery ? qsTr("%1% · %2").arg(island.status.batteryPercent).arg(island.status.batterySummary) : qsTr("No battery detected"); font.pixelSize: 16 * island.unit }
                DetailText { text: island.status && island.status.hasBattery ? island.status.batteryRemaining : ""; visible: text.length > 0; color: "#afc0cb" }
            }
            Column {
                width: parent.width; spacing: 8 * island.unit
                visible: island.section === "clock"
                DetailText { text: island.scene.clockText; font.pixelSize: 30 * island.unit; color: "white" }
                DetailText { text: island.scene.dateText }
            }
        }
    }
}
