// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

Item {
    id: face
    objectName: "sequoiaGridTestFace"
    property bool panel: false
    readonly property int columns: Math.max(1, Math.round(width / 96))
    readonly property int rows: Math.max(1, Math.round(height / 96))
    readonly property string arrangement: panel ? "Panel" : columns === 1 || rows === 1 ? "Compact"
        : columns >= 4 && rows >= 4 ? "Detailed" : columns > rows ? "Wide" : rows > columns ? "Tall" : "Square"
    readonly property string footprint: "%1 × %2".arg(columns).arg(rows)
    clip: true
    Accessible.role: Accessible.StaticText
    Accessible.name: panel ? qsTr("Grid test, panel layout") : qsTr("Grid test, %1, %2").arg(arrangement).arg(footprint)
    Rectangle {
        anchors.fill: parent
        anchors.margins: face.panel ? 0 : 6
        radius: face.panel ? 8 : 20
        color: Kirigami.Theme.backgroundColor
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.tint(Kirigami.Theme.backgroundColor, "#204c9fff") }
            GradientStop { position: 1; color: Kirigami.Theme.backgroundColor }
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: face.panel ? 2 : 12
            spacing: 8
            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: face.arrangement === "Compact" ? 20 : 32
                Layout.preferredHeight: Layout.preferredWidth
                source: "view-grid"
                visible: !face.panel
            }
            PC3.Label {
                Layout.alignment: Qt.AlignHCenter
                text: face.panel ? qsTr("Grid") : face.footprint
                font.pixelSize: face.panel ? 12 : face.arrangement === "Compact" ? 18 : 30
                font.bold: true
            }
            PC3.Label {
                Layout.alignment: Qt.AlignHCenter
                text: face.arrangement
                visible: !face.panel && face.arrangement !== "Compact"
                color: Kirigami.Theme.disabledTextColor
            }
            GridLayout {
                visible: !face.panel && face.arrangement !== "Compact" && face.arrangement !== "Square"
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: face.arrangement === "Tall" ? 1 : 3
                rowSpacing: 8; columnSpacing: 8
                Repeater {
                    model: face.arrangement === "Detailed" ? 6 : 3
                    Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 16
                        radius: 8
                        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12 + index * 0.025)
                        PC3.Label { anchors.centerIn: parent; text: index + 1 }
                    }
                }
            }
            PC3.Label {
                Layout.alignment: Qt.AlignHCenter
                visible: face.arrangement === "Detailed"
                text: qsTr("One widget. Room to grow.")
                color: Kirigami.Theme.disabledTextColor
            }
        }
    }
}
