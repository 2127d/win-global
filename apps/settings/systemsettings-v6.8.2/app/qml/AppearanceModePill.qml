// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
HubPanel {
    id: pill
    property string mode: "light"
    signal modeSelected(string mode)
    implicitWidth: 210
    implicitHeight: 42
    radius: height / 2
    tint: .35
    readonly property int activeIndex: mode === "auto" ? 2 : mode === "dark" ? 1 : 0
    readonly property real segmentWidth: (width - 8) / 3
    HubPanel {
        objectName: "appearanceModeSelection"
        x: 4 + (pill.LayoutMirroring.enabled ? 2-pill.activeIndex : pill.activeIndex) * pill.segmentWidth
        y: 4; width: pill.segmentWidth; height: pill.height - 8
        radius: height / 2
        color: Qt.alpha(K.Theme.highlightColor, .22)
        border.color: Qt.alpha(K.Theme.highlightColor, .32)
        Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
    }
    RowLayout {
        anchors.fill: parent; anchors.margins: 4; spacing: 0
        Repeater {
            model: [{mode:"light",text:qsTr("Light")},{mode:"dark",text:qsTr("Dark")},{mode:"auto",text:qsTr("Auto")}]
            C.AbstractButton {
                id: segment
                required property var modelData
                objectName: "appearanceMode-" + modelData.mode
                Layout.fillWidth:true; Layout.fillHeight:true; Layout.preferredWidth:1; Layout.minimumWidth:0
                text:modelData.text; hoverEnabled:true
                Accessible.role:Accessible.RadioButton
                Accessible.checked:pill.mode===modelData.mode
                background:HubPanel {
                    radius:height/2
                    color:Qt.alpha(K.Theme.highlightColor,segment.down ? .18 : .09)
                    opacity:segment.hovered || segment.activeFocus ? 1 : 0
                    emphasized:segment.activeFocus
                    Behavior on opacity { NumberAnimation { duration:120 } }
                }
                contentItem:C.Label { text:segment.text; color:K.Theme.textColor; horizontalAlignment:Text.AlignHCenter; verticalAlignment:Text.AlignVCenter; font.weight:pill.mode===segment.modelData.mode ? Font.DemiBold : Font.Normal }
                onClicked:pill.modeSelected(modelData.mode)
                C.ToolTip.visible:hovered && modelData.mode==="auto"
                C.ToolTip.text:qsTr("Configure automatic light/dark themes in Global Theme")
            }
        }
    }
}
