// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    readonly property real naturalHeight: content.implicitHeight + islandRow.implicitHeight + 36
    required property var controller
    objectName: "audioSpotlightView"
    property bool applicationsExpanded: false
    readonly property string layoutMode: ["spotlight","channels","mixer"].includes(controller.layoutMode) ? controller.layoutMode : "spotlight"
    readonly property bool showApps: applicationsExpanded || layoutMode === "channels"
    onLayoutModeChanged: { if (scroll.contentItem) scroll.contentItem.contentY = 0; }
    readonly property var output: controller.outputs.find(d => d.id === controller.outputId) || controller.outputs[0] || null
    readonly property var input: controller.inputs.find(d => d.id === controller.inputId) || controller.inputs[0] || null
    readonly property var streams: controller.playback.concat(controller.recording)
    PC.ScrollView {
        id: scroll
        objectName: "audioDeviceScroll"
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: -6; anchors.rightMargin: -6
        anchors.bottom: island.top; anchors.bottomMargin: -8
        background: null
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
        layer.effect: MultiEffect { maskEnabled: true; maskSource: edgeMask; maskThresholdMin: 0.5; maskSpreadAtMin: 1; autoPaddingEnabled: false }
        contentItem: Flickable {
            clip: true; contentWidth: width; contentHeight: content.implicitHeight + 40
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content
                x: 6; y: 6; width: Math.max(0,scroll.availableWidth - 12)
                spacing: 12
                Loader {
                    Layout.fillWidth: true
                    active: view.layoutMode === "spotlight" && !!view.output
                    visible: active
                    sourceComponent: AudioDeviceCard { record: view.output || {}; devices: view.controller.outputs; controller: view.controller }
                }
                GridLayout {
                    visible: view.layoutMode === "channels"
                    Layout.fillWidth: true; columns: view.output && view.input ? 2 : 1; columnSpacing: 12
                    Loader {
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                        active: view.layoutMode === "channels" && !!view.output; visible: active
                        sourceComponent: AudioChannelCard { record: view.output || {}; devices: view.controller.outputs; controller: view.controller }
                    }
                    Loader {
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                        active: view.layoutMode === "channels" && !!view.input; visible: active
                        sourceComponent: AudioChannelCard { record: view.input || {}; devices: view.controller.inputs; controller: view.controller; input: true }
                    }
                }
                Loader {
                    Layout.fillWidth: true
                    active: view.layoutMode === "mixer" && !!view.output; visible: active
                    sourceComponent: AudioMixerCard { record: view.output || {}; controller: view.controller; master: true }
                }
                PC.Label { visible: view.layoutMode === "mixer"; text: qsTr("Apps & destinations"); color: Kirigami.Theme.disabledTextColor; Layout.leftMargin: 10 }
                Repeater {
                    model: view.layoutMode === "mixer" ? view.streams.length : 0
                    delegate: AudioMixerCard { required property int index; readonly property var modelData: view.streams[index]; Layout.fillWidth: true; record: modelData; controller: view.controller }
                }
                PC.Label { visible: view.layoutMode === "mixer" && !view.streams.length; Layout.fillWidth: true; Layout.leftMargin: 10; text: qsTr("No apps are using audio"); wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
                Details.DetailSurface {
                    visible: !view.output
                    Layout.fillWidth: true
                    implicitHeight: emptyContent.implicitHeight + 32; radius: 24
                    ColumnLayout {
                        id: emptyContent; anchors.fill: parent; anchors.margins: 16; spacing: 12
                        Kirigami.Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 40; Layout.preferredHeight: 40; source: "audio-volume-muted-symbolic" }
                        PC.Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; text: view.controller.ready ? qsTr("No output device available") : qsTr("Connecting to the sound service…") }
                        AudioActionButton { visible: !view.controller.ready; Layout.alignment: Qt.AlignHCenter; text: qsTr("Retry"); icon.name: "view-refresh-symbolic"; onClicked: view.controller.retry() }
                    }
                }
                Loader {
                    Layout.fillWidth: true
                    active: view.layoutMode !== "channels" && !!view.input; visible: active
                    sourceComponent: AudioDeviceCard { record: view.input || {}; devices: view.controller.inputs; controller: view.controller; input: true }
                }
                Details.DetailSurface {
                    Layout.fillWidth: true; radius: 24
                    visible: view.layoutMode !== "mixer"
                    implicitHeight: appContent.implicitHeight + 24
                    ColumnLayout {
                        id: appContent
                        anchors.fill: parent; anchors.margins: 12; spacing: 10
                        AudioActionButton {
                            objectName: "audioAppsToggle"
                            Layout.fillWidth: true; flat: true
                            text: qsTr("App volumes · %1").arg(view.streams.length)
                            icon.name: view.showApps ? "arrow-up-symbolic" : "arrow-down-symbolic"
                            Accessible.name: text
                            enabled: view.layoutMode !== "channels"
                            onClicked: view.applicationsExpanded = !view.applicationsExpanded
                        }
                        PC.Label { Layout.fillWidth: true; visible: view.showApps && !view.streams.length; text: qsTr("No apps are using audio"); color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap }
                        Repeater {
                            model: view.layoutMode !== "mixer" && view.showApps ? view.streams.length : 0
                            delegate: ColumnLayout {
                                required property int index
                                readonly property var modelData: view.streams[index]
                                Layout.fillWidth: true; spacing: 3
                                objectName: "audioApp:" + modelData.id
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 8
                                    Kirigami.Icon { Layout.preferredWidth: 24; Layout.preferredHeight: 24; source: modelData.icon || (modelData.type === "source-output" ? "audio-input-microphone-symbolic" : "application-x-executable") }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 0
                                        PC.Label { Layout.fillWidth: true; text: modelData.name; textFormat: Text.PlainText; elide: Text.ElideRight }
                                        PC.Label { visible: modelData.type === "source-output"; text: qsTr("Microphone access"); color: Kirigami.Theme.disabledTextColor; font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.85 }
                                    }
                                    PC.Label { text: modelData.muted ? qsTr("Muted") : qsTr("%1%").arg(Math.round(modelData.volume)); color: Kirigami.Theme.disabledTextColor }
                                    AudioActionButton { flat: true; icon.name: "view-more-symbolic"; text: qsTr("App audio options"); display: PC.AbstractButton.IconOnly; Accessible.name: text; onClicked: view.controller.options(modelData.id,this); PC.ToolTip { text: parent.text } }
                                }
                                AudioVolumeRow { Layout.fillWidth: true; record: modelData; controller: view.controller; input: modelData.type === "source-output" }
                            }
                        }
                    }
                }
            }
        }
    }
    AudioMenu {
        id: layoutMenu
        objectName: "audioLayoutMenu"
        AudioMenuItem { objectName: "audioLayoutSpotlight"; text: qsTr("Output spotlight"); checkable: true; checked: view.layoutMode === "spotlight"; onTriggered: view.controller.setLayout("spotlight") }
        AudioMenuItem { objectName: "audioLayoutChannels"; text: qsTr("Twin channels"); checkable: true; checked: view.layoutMode === "channels"; onTriggered: view.controller.setLayout("channels") }
        AudioMenuItem { objectName: "audioLayoutMixer"; text: qsTr("Mixer desk"); checkable: true; checked: view.layoutMode === "mixer"; onTriggered: view.controller.setLayout("mixer") }
    }
    Rectangle {
        id: edgeMask
        width: scroll.width; height: scroll.height
        visible: false; layer.enabled: true
        readonly property real fade: Math.min(0.12, 18 / Math.max(1,height))
        gradient: Gradient {
            GradientStop { position: 0; color: scroll.contentItem.contentY > 2 ? "transparent" : "white" }
            GradientStop { position: edgeMask.fade; color: "white" }
            GradientStop { position: 1 - edgeMask.fade; color: "white" }
            GradientStop { position: 1; color: "transparent" }
        }
    }
    Details.DetailSurface {
        id: island
        objectName: "audioActionIsland"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 8
        width: Math.min(parent.width - 12, islandRow.implicitWidth + 16)
        height: islandRow.implicitHeight + 10
        radius: height / 2; elevated: true; z: 5
        MouseArea { anchors.fill: parent }
        RowLayout {
            id: islandRow
            anchors.fill: parent; anchors.margins: 5; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 3
            AudioActionButton { objectName: "audioSettings"; flat: true; text: qsTr("Sound settings"); icon.name: "configure-symbolic"; enabled: view.controller.settingsAllowed; onClicked: view.controller.settings() }
            AudioActionButton {
                id: layoutButton
                objectName: "audioLayoutPicker"
                flat: true; text: qsTr("Layout"); icon.name: "view-grid-symbolic"
                display: view.width < 350 ? PC.AbstractButton.IconOnly : PC.AbstractButton.TextBesideIcon; Accessible.name: qsTr("Audio layout")
                onClicked: layoutMenu.popup(layoutButton,0,0)
                PC.ToolTip { text: qsTr("Audio layout") }
            }
            AudioActionButton { objectName: "audioMore"; flat: true; text: qsTr("More audio controls"); icon.name: "view-more-symbolic"; display: PC.AbstractButton.IconOnly; Accessible.name: text; onClicked: view.controller.more(this); PC.ToolTip { text: parent.text } }
        }
    }
}
