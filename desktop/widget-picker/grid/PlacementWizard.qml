// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

FocusScope {
    id: wizard
    objectName: "sequoiaPlacementWizard"
    focus: true
    Keys.onEscapePressed: event => { GridSession.cancel(); event.accepted = true; }
    readonly property var desktops: GridSession.targets.filter(t => t && t.kind === "desktop" && t.screen >= 0)
    readonly property var panels: GridSession.targets.filter(t => t && t.kind === "panel" && t.screen >= 0)
    readonly property real leftEdge: Math.min(0, ...desktops.map(t => t.geometry.x))
    readonly property real topEdge: Math.min(0, ...desktops.map(t => t.geometry.y))
    readonly property real mapWidth: Math.max(1, ...desktops.map(t => t.geometry.x + t.geometry.width)) - leftEdge
    readonly property real mapHeight: Math.max(1, ...desktops.map(t => t.geometry.y + t.geometry.height)) - topEdge
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            PC3.ToolButton {
                text: qsTr("Back")
                icon.name: "go-previous"
                onClicked: GridSession.phase === "preset" ? GridSession.phase = "destination" : GridSession.cancel()
            }
            Item { Layout.fillWidth: true }
            PC3.ToolButton { text: qsTr("Cancel"); onClicked: GridSession.cancel() }
        }
        Kirigami.Heading {
            Layout.fillWidth: true
            text: GridSession.phase === "destination" ? qsTr("Where should %1 go?").arg(GridSession.title) : qsTr("Choose a layout")
            level: 1
            wrapMode: Text.WordWrap
        }
        PC3.Label {
            Layout.fillWidth: true
            text: GridSession.phase === "destination" ? qsTr("Choose a desktop or one of your panels.")
                : qsTr("Start with a preset. Resize it on the desktop to change its layout.")
            wrapMode: Text.WordWrap
            color: Kirigami.Theme.disabledTextColor
        }
        Item {
            id: screenMap
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: GridSession.phase === "destination"
            readonly property real factor: Math.min(width / wizard.mapWidth, height / wizard.mapHeight) * 0.9
            Item {
                x: (screenMap.width - wizard.mapWidth * screenMap.factor) / 2
                y: (screenMap.height - wizard.mapHeight * screenMap.factor) / 2
                Repeater {
                    model: wizard.desktops
                    Rectangle {
                        required property var modelData
                        x: (modelData.geometry.x - wizard.leftEdge) * screenMap.factor + 3
                        y: (modelData.geometry.y - wizard.topEdge) * screenMap.factor + 3
                        width: Math.max(32, modelData.geometry.width * screenMap.factor - 6)
                        height: Math.max(32, modelData.geometry.height * screenMap.factor - 6)
                        radius: 14
                        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.10)
                        border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.6)
                        PC3.Button {
                            anchors.centerIn: parent
                            width: Math.min(implicitWidth, parent.width - 20)
                            text: modelData.label
                            enabled: modelData.ready
                            onClicked: GridSession.choose(modelData)
                        }
                    }
                }
                Repeater {
                    model: wizard.panels
                    PC3.Button {
                        required property var modelData
                        x: (modelData.geometry.x - wizard.leftEdge) * screenMap.factor
                        y: (modelData.geometry.y - wizard.topEdge) * screenMap.factor
                        width: Math.max(18, modelData.geometry.width * screenMap.factor)
                        height: Math.max(18, modelData.geometry.height * screenMap.factor)
                        text: ""
                        Accessible.name: modelData.label
                        enabled: modelData.ready && GridSession.definition.panel
                        PC3.ToolTip.visible: hovered
                        PC3.ToolTip.text: modelData.label
                        onClicked: GridSession.choose(modelData)
                    }
                }
            }
        }
        // Named buttons also make small and overlapping panels accessible.
        Flow {
            visible: GridSession.phase === "destination"
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 8
            Repeater {
                model: wizard.panels
                PC3.Button {
                    required property var modelData
                    text: modelData.label
                    enabled: modelData.ready && GridSession.definition.panel
                    onClicked: GridSession.choose(modelData)
                }
            }
        }
        Item {
            id: previewArea
            visible: GridSession.phase === "preset"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Item {
                width: GridSession.preset.columns * GridSession.cell
                height: GridSession.preset.rows * GridSession.cell
                anchors.centerIn: parent
                scale: Math.min(1, (previewArea.width - 24) / width, (previewArea.height - 24) / height)
                Loader {
                    anchors.fill: parent
                    source: GridSession.definition.preview || ""
                    active: GridSession.phase === "preset" && !!GridSession.definition.preview
                    onLoaded: if ("presetConfiguration" in item) {
                        item.presetConfiguration = Qt.binding(() => GridSession.preset.configuration || {});
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    visible: !GridSession.definition.preview
                    radius: 18
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
                    Kirigami.Icon { anchors.centerIn: parent; width: 64; height: 64; source: GridSession.icon }
                }
            }
        }
        PC3.Label {
            visible: GridSession.phase === "preset"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("%1 · %2 × %3").arg(GridSession.preset.name).arg(GridSession.preset.columns).arg(GridSession.preset.rows)
            font.bold: true
        }
        RowLayout {
            visible: GridSession.phase === "preset"
            Layout.alignment: Qt.AlignHCenter
            spacing: 16
            PC3.Button {
                text: qsTr("Previous")
                enabled: GridSession.presetIndex > 0
                onClicked: GridSession.presetIndex--
            }
            PC3.Label { text: qsTr("%1 of %2").arg(GridSession.presetIndex + 1).arg(GridSession.definition.presets.length) }
            PC3.Button {
                text: qsTr("Next")
                enabled: GridSession.presetIndex + 1 < GridSession.definition.presets.length
                onClicked: GridSession.presetIndex++
            }
        }
        PC3.Button {
            objectName: "sequoiaUsePreset"
            visible: GridSession.phase === "preset"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Use this preset")
            icon.name: "object-select"
            enabled: GridSession.destination !== null && GridSession.destination.ready
                && GridSession.preset.columns <= GridSession.destination.columns
                && GridSession.preset.rows <= GridSession.destination.rows
            onClicked: GridSession.place()
        }
        PC3.Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: GridSession.error
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.WordWrap
        }
    }
}
