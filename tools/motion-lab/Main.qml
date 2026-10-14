import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: lab
    visible: true; width: 1190; height: 890; minimumWidth: 980; minimumHeight: 700
    title: "Sequoia Motion Lab"
    color: "#f2f5f9"
    property int categoryIndex: 0
    property bool reducedMotion: false
    property bool slowPlayback: false
    property string saveMessage: "Candidates save locally for later review. Desktop settings stay unchanged."
    readonly property var category: motionData.categories[categoryIndex]
    palette.window: "#f2f5f9"; palette.windowText: "#24344c"; palette.text: "#24344c"
    palette.button: "#edf2f9"; palette.buttonText: "#24344c"; palette.base: "white"
    palette.highlight: "#3d75cf"; palette.highlightedText: "white"
    font.family: "Sans Serif"
    function replayBoth() { left.replay(); right.replay(); }
    onCategoryIndexChanged: if (loop.checked) loopTimer.restart()
    Shortcut { sequence: "Ctrl+R"; onActivated: lab.replayBoth() }
    Timer { id: loopTimer; interval: Math.max(left.currentDuration, right.currentDuration) * (lab.slowPlayback ? 4 : 1) + 1000; running: loop.checked; repeat: true; onTriggered: lab.replayBoth() }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 30; spacing: 18
        RowLayout {
            ColumnLayout {
                spacing: 7; Layout.fillWidth: true
                Text { text: "SEQUOIA / MOTION STUDIES"; color: "#5b77a2"; font.pixelSize: 11; font.letterSpacing: 2; font.bold: true }
                Text { text: "One language. Different rhythms."; color: "#1d2d46"; font.pixelSize: 29; font.weight: Font.DemiBold }
                Text { text: "Compare motion by purpose, then choose what feels right."; color: "#66758b"; font.pixelSize: 14 }
            }
            Rectangle { color: "#e0ebe6"; radius: 12; implicitWidth: 112; implicitHeight: 29
                Text { anchors.centerIn: parent; text: "PREVIEW ONLY"; color: "#366454"; font.pixelSize: 10; font.bold: true }
            }
        }
        RowLayout {
            spacing: 12
            Button { text: "▶  Replay both"; highlighted: true; onClicked: lab.replayBoth() }
            CheckBox { id: loop; text: "Repeat"; onToggled: if (checked) lab.replayBoth() }
            CheckBox { text: "Slow playback (¼×)"; checked: lab.slowPlayback; onToggled: { lab.slowPlayback = checked; lab.replayBoth(); } }
            CheckBox { text: "Reduced motion"; checked: lab.reducedMotion; onToggled: lab.reducedMotion = checked }
            Item { Layout.fillWidth: true }
            Text { text: "Ctrl+R to replay"; color: "#7a879a"; font.pixelSize: 12 }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: availableWidth
            RowLayout {
                width: parent.width; spacing: 20
                ColumnLayout {
                    Layout.preferredWidth: 215; Layout.maximumWidth: 215; Layout.alignment: Qt.AlignTop; spacing: 7
                    Text { text: "MOTION FAMILIES"; color: "#7a879a"; font.pixelSize: 10; font.letterSpacing: 1.3; Layout.bottomMargin: 7 }
                    Repeater {
                        model: motionData.categories
                        delegate: Button {
                            required property var modelData; required property int index
                            Layout.fillWidth: true; implicitHeight: 46
                            text: modelData.name; checked: lab.categoryIndex === index
                            onClicked: { lab.categoryIndex = index; left.resetSample(); right.resetSample(); }
                            background: Rectangle { color: parent.checked ? "#dce7f8" : parent.hovered ? "#e9eef5" : "transparent"; radius: 10 }
                            contentItem: Text { text: parent.text; verticalAlignment: Text.AlignVCenter; leftPadding: 12; color: parent.checked ? "#25599f" : "#55647b"; font.pixelSize: 13; font.bold: parent.checked }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: "#dce2eb"; Layout.topMargin: 14; Layout.bottomMargin: 10 }
                    Text { text: "THE SHARED RULE"; color: "#7a879a"; font.pixelSize: 10; font.letterSpacing: 1.1 }
                    Text { Layout.fillWidth: true; text: "Small actions respond quickly. Bigger changes get more time. Exits are shorter; direct input stays immediate."; color: "#66758b"; font.pixelSize: 13; wrapMode: Text.WordWrap; lineHeight: 1.3 }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignTop; spacing: 12
                    Text { text: lab.category.name; color: "#243650"; font.pixelSize: 23; font.weight: Font.DemiBold }
                    Text { text: lab.category.examples; color: "#697b94"; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 16
                        PalettePanel {
                            id: left; objectName: "paletteA"; side: "A"; category: lab.categoryIndex
                            Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.alignment: Qt.AlignTop
                            reduced: lab.reducedMotion; playbackScale: lab.slowPlayback ? 4 : 1
                            onSaved: message => lab.saveMessage = message
                        }
                        PalettePanel {
                            id: right; objectName: "paletteB"; side: "B"; category: lab.categoryIndex
                            Layout.fillWidth: true; Layout.preferredWidth: 1; Layout.alignment: Qt.AlignTop
                            reduced: lab.reducedMotion; playbackScale: lab.slowPlayback ? 4 : 1
                            onSaved: message => lab.saveMessage = message
                        }
                    }
                    Text { text: lab.category.rule; color: "#586c88"; font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.WordWrap; lineHeight: 1.2 }
                    Text { text: "Representative scenes: " + lab.category.shape + ". System integration comes after you choose a palette."; color: "#7a879a"; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
            }
        }
        Text { text: lab.saveMessage; color: "#617590"; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
    }
}
