// SPDX-License-Identifier: MIT
import QtQuick
import "../details" as Details
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.private.volume

PlasmaExtras.Representation {
    id: root
    required property var applet
    required property var audioConfig
    property alias microphoneTestPage: microphoneTest
    objectName: "sequoiaDetailPage"
    padding: 0; background: null
    collapseMarginsHint: true
    implicitWidth: 418; implicitHeight: 510
    readonly property real sequoiaPreferredHeight: applet.micTestActive ? 430 : spotlight.naturalHeight
    Layout.minimumWidth: 280
    Layout.minimumHeight: 300
    Keys.onEscapePressed: event => {
        if (applet.micTestActive) { applet.micTestActive = false; microphoneTest.stopTest(); event.accepted = true; }
        else event.accepted = false;
    }
    AudioRecords { id: outputsRecords; pulseModel: root.applet.paSinkFilterModel; kind: "sink" }
    AudioRecords { id: inputsRecords; pulseModel: root.applet.paSourceFilterModel; kind: "source" }
    AudioRecords { id: playbackRecords; pulseModel: root.applet.paSinkInputFilterModel; kind: "sink-input" }
    AudioRecords { id: recordingRecords; pulseModel: root.applet.paSourceOutputFilterModel; kind: "source-output" }
    function record(id) { return outputsRecords.entries.concat(inputsRecords.entries,playbackRecords.entries,recordingRecords.entries).find(d => d.id === id) || null; }
    function stopMicrophoneTest() { root.applet.micTestActive = false; microphoneTest.stopTest(); }
    Connections {
        target: root.applet
        function onExpandedChanged() { if (!root.applet.expanded) root.stopMicrophoneTest(); }
    }
    Connections {
        target: inputsRecords
        function onEntriesChanged() {
            if (root.applet.micTestActive && !inputsRecords.entries.some(d => d.object === microphoneTest.sourceObject)) root.stopMicrophoneTest();
        }
    }
    QtObject {
        id: nativeController
        readonly property string layoutMode: Plasmoid.configuration.audioLayout || "spotlight"
        function setLayout(mode) {
            if (!["spotlight","channels","mixer"].includes(mode)) return;
            Plasmoid.configuration.audioLayout = mode;
            Plasmoid.configuration.writeConfig();
        }
        function routeStream(id, destinationId) {
            const stream = root.record(id);
            const device = root.record(destinationId);
            if (!stream?.object || !device?.object) return;
            const type = stream.type === "sink-input" ? "sink" : stream.type === "source-output" ? "source" : "";
            if (device.type === type) stream.object.deviceIndex = device.index;
        }
        readonly property var outputs: outputsRecords.entries
        readonly property var inputs: inputsRecords.entries
        readonly property var playback: playbackRecords.entries
        readonly property var recording: recordingRecords.entries
        readonly property string outputId: PreferredDevice.sink ? "sink:" + PreferredDevice.sink.index : ""
        readonly property string inputId: PreferredDevice.source ? "source:" + PreferredDevice.source.index : ""
        readonly property bool ready: root.applet.appletContextReady
        readonly property real maximumVolume: root.audioConfig.raiseMaximumVolume ? PulseAudio.MaximalVolume / PulseAudio.NormalVolume * 100 : 100
        readonly property int volumeStep: root.audioConfig.volumeStep
        readonly property bool settingsAllowed: root.applet.audioSettingsAllowed
        function setVolume(id, value) {
            const item = root.record(id);
            if (!item?.object || !item.hasVolume || !item.writable || !Number.isFinite(value)) return;
            const limit = Math.max(maximumVolume,item.volume);
            const percent = Math.max(0, Math.min(limit, value));
            item.object.volume = Math.round(percent * PulseAudio.NormalVolume / 100);
            item.object.muted = percent === 0;
        }
        function toggleMute(id) { const item = root.record(id); if (item?.object) item.object.muted = !item.object.muted; }
        function selectDevice(id) {
            const item = root.record(id);
            if (item?.object && ["sink","source"].includes(item.type)) item.object.default = true;
        }
        function feedback(id) { const item = root.record(id); if (item?.type === "sink") root.applet.playFeedback(item.index); }
        function retry() { root.applet.retryAudioService(); }
        function settings() { root.applet.openAudioSettings(); }
        function options(id, button) { deviceOptions.openFor(id,button); }
        function cardFor(item) {
            const cards = root.applet.paCardModel;
            const index = cards.indexOfCardNumber(item.object.cardIndex);
            return cards.data(index,cards.KItemModels.KRoleNames.role("PulseObject"));
        }
        function optionEntries(id) {
            const item = root.record(id); if (!item?.object) return [];
            const result = [];
            const devices = item.type.startsWith("sink") ? outputs : inputs;
            const device = item.type === "sink" || item.type === "source";
            if (!device) {
                if (devices.length > 1) {
                    result.push({label: item.type === "sink-input" ? qsTr("Play audio using") : qsTr("Record audio using"),section:true});
                    for (const target of devices) result.push({key:"route:"+target.id,label:target.name,action:"route",targetId:target.id,checkable:true,checked:item.deviceIndex===target.index});
                }
            } else {
                if (devices.length > 1) result.push({key:"all",label:item.type === "sink" ? qsTr("Play all audio via this device") : qsTr("Record all audio via this device"),action:"all",icon:item.type === "sink" ? "audio-speakers-symbolic" : "audio-input-microphone-symbolic"});
                const ports = item.object.ports || [];
                const activeUnavailable = ports[item.object.activePortIndex]?.availability === 2;
                const choices = [];
                for (let i = 0; i < ports.length; ++i) {
                    const port = ports[i];
                    if (!port || (!activeUnavailable && port.availability === 2)) continue;
                    choices.push({key:"port:"+port.name,label:port.description + (port.availability === 2 ? qsTr(" (unavailable)") : ""),action:"port",name:port.name,checkable:true,checked:i===item.object.activePortIndex});
                }
                if (choices.length > 1) result.push({label:qsTr("Ports"),section:true},...choices);
                const card = cardFor(item);
                const profiles = card?.profiles || [];
                const profileChoices = [];
                for (let i = 0; i < profiles.length; ++i) {
                    const profile = profiles[i];
                    if (!profile || profile.availability === 2 || profile.name === "off") continue;
                    profileChoices.push({key:"profile:"+profile.name,label:profile.description,action:"profile",name:profile.name,checkable:true,checked:i===card.activeProfileIndex});
                }
                if (profileChoices.length > 1) result.push({label:qsTr("Profiles"),section:true},...profileChoices);
            }
            if (!result.length) result.push({label:qsTr("No additional options"),section:true});
            return result;
        }
        function activateOption(id, entry) {
            const item = root.record(id); if (!item?.object || entry.section) return;
            if (entry.action === "route") { routeStream(id,entry.targetId); return; }
            if (!["sink","source"].includes(item.type)) return;
            if (entry.action === "all") item.object.switchStreams();
            else if (entry.action === "port") {
                const ports = item.object.ports || [];
                for (let i=0; i<ports.length; ++i) if (ports[i]?.name === entry.name) {item.object.activePortIndex=i;break;}
            } else if (entry.action === "profile") {
                const card=cardFor(item);const profiles=card?.profiles || [];
                for (let i=0; i<profiles.length; ++i) if (profiles[i]?.name === entry.name && profiles[i].availability !== 2 && profiles[i].name !== "off") {card.activeProfileIndex=i;break;}
            }
        }
        function more(button) { moreMenu.popup(button,0,0); }
    }
    AudioOptionsMenu { id: deviceOptions; controller: nativeController }
    AudioMenu {
        id: moreMenu
        objectName: "audioMoreMenu"
        AudioMenuItem {
            text: qsTr("Test microphone…")
            enabled: !!spotlight.input
            onTriggered: {
                const input = root.record(nativeController.inputId) || inputsRecords.entries[0];
                if (!input?.object) return;
                microphoneTest.model = {PulseObject:input.object,Description:input.name,IconName:input.icon,Ports:input.object.ports,ActivePortIndex:input.object.activePortIndex};
                root.applet.micTestActive = true;
            }
        }
        AudioMenuSeparator {}
        AudioMenuItem {
            text: qsTr("Raise maximum volume")
            checkable: true; checked: root.audioConfig.raiseMaximumVolume
            onTriggered: { root.audioConfig.raiseMaximumVolume = !root.audioConfig.raiseMaximumVolume; root.audioConfig.save(); }
        }
        AudioMenuItem {
            text: qsTr("Show virtual devices")
            checkable: true; checked: root.applet.showVirtualDevices
            onTriggered: Plasmoid.configuration.showVirtualDevices = !root.applet.showVirtualDevices
        }
    }
    Details.DetailPageDeck {
        objectName: "audioNavigation"
        anchors.fill: parent
        currentIndex: root.applet.micTestActive ? 1 : 0
        pages: [
            AudioSpotlightView { id: spotlight; controller: nativeController },
            MicrophoneTestView {
                id: microphoneTest
                showInternalBackButton: !(Plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading)
                onBackRequested: root.stopMicrophoneTest()
            }
        ]
    }
}
