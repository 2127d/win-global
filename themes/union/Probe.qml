// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.union.impl as Union

ApplicationWindow {
    id: window
    width: 660; height: 430
    visible: true
    title: "Sequoia · Union feasibility"
    property int activations: 0
    property var report: ({
        windowAlpha: color.a,
        cardRadius: card.Union.Style.properties.corners.topLeft.radius,
        cardAlpha: card.Union.Style.properties.background.color.a,
        glassAlpha: glass.Union.Style.properties.background.color.a,
        buttonRadius: action.Union.Style.properties.corners.topLeft.radius,
        shadowBlur: card.Union.Style.properties.shadow.blur,
        shadowAlpha: card.Union.Style.properties.shadow.color.a
    })
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        Label { text: "Sequoia / Union"; font.pixelSize: 24; font.weight: Font.DemiBold }
        Label { text: "One CSS style · native Qt Quick controls" }
        Frame {
            id: card
            objectName: "card"
            Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent; spacing: 12
                Label { text: "Opaque content island"; font.weight: Font.DemiBold }
                RowLayout {
                    Button { id: action; objectName: "action"; text: "Action"; onClicked: window.activations++ }
                    Button { text: "Selected"; checkable: true; checked: true }
                    Button { objectName: "disabled"; text: "Unavailable"; enabled: false; onClicked: window.activations++ }
                }
                TextField { placeholderText: "Search desktop surfaces"; Layout.fillWidth: true }
                RowLayout {
                    Switch { text: "Notifications"; checked: true }
                    Slider { value: 0.65; Layout.fillWidth: true }
                }
            }
        }
        Frame {
            id: glass
            Union.Element.elementId: "glass-probe"
            Layout.fillWidth: true
            implicitHeight: 65
            Label { anchors.centerIn: parent; text: "72% material · compositor blur tested separately" }
        }
    }
}
