// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: win; width: 760; height: 560; visible: true; title: "Sequoia standard Qt Quick controls"
    property int activations: 0
    ColumnLayout { anchors.fill: parent; anchors.margins: 26; spacing: 18
        Label { text: "Standard Qt Quick controls"; font.pixelSize: 24 }
        Label { text: "No Sequoia component imports or application makeover." }
        RowLayout {
            Button { objectName: "action"; text: "Ordinary button"; onClicked: win.activations++ }
            Button { text: "Checked"; checkable: true; checked: true }
            Button { objectName: "disabled"; text: "Disabled"; enabled: false; onClicked: win.activations++ }
        }
        TextField { objectName: "field"; Layout.fillWidth: true; placeholderText: "Type in a standard text field" }
        RowLayout {
            ComboBox { objectName: "combo"; model: ["First option", "Second option", "Third option"] }
            SpinBox { objectName: "spin"; value: 24 }
            ToolButton { text: "Tool button" }
        }
        RowLayout { CheckBox { objectName: "check"; text: "Checkbox" }
            RadioButton { text: "Radio button"; checked: true }
            Switch { text: "Switch"; checked: true }
        }
        Slider { objectName: "slider"; Layout.fillWidth: true; from: 0; to: 100; stepSize: 1; value: 60 }
        ProgressBar { Layout.fillWidth: true; value: .60 }
        Item { Layout.fillHeight: true }
    }
}
