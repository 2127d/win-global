import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    required property string side
    required property int category
    required property bool reduced
    required property real playbackScale
    property int presetIndex: side === "A" ? 0 : 1
    property var durations: motionData.presets[presetIndex].durations.slice()
    readonly property var preset: motionData.presets[presetIndex]
    readonly property int currentDuration: durations[category]
    signal saved(string message)
    color: "white"; radius: 18; border.color: "#dce2eb"
    implicitHeight: content.implicitHeight + 40
    onPresetIndexChanged: { durations = motionData.presets[presetIndex].durations.slice(); sample.reset(); }
    function replay() { sample.reset(); sample.play(); }
    function resetSample() { sample.reset(); }
    function exportCandidate() {
        return JSON.stringify({schemaVersion: 1, name: preset.name, durations: durations,
            distance: preset.distance, curve: preset.curve, reducedMotion: reduced,
            exitRatio: .7, symmetricCategories: ["navigation", "workspace"],
            reducedMotionPolicy: "No travel, scale or pulse; fades capped at 80ms; direct input stays immediate",
            categories: motionData.categories, scope: "preview candidate; no system settings applied"});
    }
    ColumnLayout {
        id: content; anchors.fill: parent; anchors.margins: 20; spacing: 14
        RowLayout {
            Text { text: panel.side; color: "#5475a8"; font.pixelSize: 13; font.bold: true }
            ComboBox {
                Layout.fillWidth: true; model: motionData.presets; textRole: "name"
                currentIndex: panel.presetIndex
                onActivated: {
                    panel.presetIndex = currentIndex;
                    panel.durations = motionData.presets[currentIndex].durations.slice();
                    sample.reset();
                }
                Accessible.name: "Palette " + panel.side
            }
        }
        Text { text: panel.preset.description; color: "#66758b"; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        MotionSample {
            id: sample; objectName: "sample" + panel.side
            Layout.fillWidth: true; Layout.preferredHeight: 256
            category: panel.category; entryDuration: panel.currentDuration; distance: panel.preset.distance
            curve: panel.preset.curve; reduced: panel.reduced; playbackScale: panel.playbackScale
        }
        Text { text: "Click the scene or press Space to toggle."; color: "#6e7d92"; font.pixelSize: 12 }
        RowLayout {
            Text { text: "Entry / change"; color: "#293d58"; font.pixelSize: 13; Layout.fillWidth: true }
            SpinBox {
                id: durationInput; from: 0; to: 1000; stepSize: 10; editable: true; value: panel.currentDuration
                Accessible.name: "Duration in milliseconds for palette " + panel.side
                onValueModified: { let copy = panel.durations.slice(); copy[panel.category] = value; panel.durations = copy; panel.replay(); }
            }
            Text { text: "ms"; color: "#6e7d92"; font.pixelSize: 12 }
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#6e7d92"; font.pixelSize: 12
            text: (panel.category === 3 || panel.category === 5 ? "Both directions: " : "Exit: ")
                + Math.round(panel.currentDuration * (panel.category === 3 || panel.category === 5 ? 1 : .7)) + " ms · "
                + (panel.category === 3 || panel.category === 5 ? "InOutCubic" : panel.preset.curve + " in / InCubic out")
                + (panel.reduced ? "\nReduced motion: no movement; fade ≤ 80 ms." : "")
        }
        RowLayout {
            Button { text: "Replay " + panel.side; Layout.fillWidth: true; onClicked: panel.replay() }
            Button { text: "Save " + panel.side + " as candidate"; Layout.fillWidth: true; onClicked: panel.saved(candidateStore.save(panel.exportCandidate())) }
        }
    }
}
