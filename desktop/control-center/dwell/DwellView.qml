// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    objectName: "dwellView"
    required property var controller
    readonly property real naturalHeight: content.implicitHeight + (footer.visible ? footer.implicitHeight + 12 : 0) + 8
    readonly property var modes: [
        {id:10,label:qsTr("Left-click"),icon:"input-mouse-click-left"},
        {id:11,label:qsTr("Double-click"),icon:"input-mouse-click-left-double"},
        {id:12,label:qsTr("Click and drag"),icon:"input-mouse"},
        {id:13,label:qsTr("Middle-click"),icon:"input-mouse-click-middle"},
        {id:14,label:qsTr("Right-click"),icon:"input-mouse-click-right"}
    ]
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PC.ScrollView {
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footer.visible ? footer.top : parent.bottom; bottomMargin: footer.visible ? 12 : 0 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true; contentWidth: width; contentHeight: content.implicitHeight + 8
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content; width: parent.width; spacing: 12
                Details.DetailSurface {
                    objectName: "dwellStatusCard"
                    Layout.fillWidth: true; radius: 22; implicitHeight: statusBody.implicitHeight + 28
                    ColumnLayout {
                        id: statusBody; anchors.fill: parent; anchors.margins: 14; spacing: 10
                        RowLayout {
                            Layout.fillWidth: true; spacing: 12
                            Kirigami.Icon { Layout.preferredWidth: 30; Layout.preferredHeight: 30; source: "input-mouse" }
                            PC.Label {
                                objectName: "dwellStatus"
                                Layout.fillWidth: true; wrapMode: Text.Wrap; font.weight: Font.DemiBold
                                text: !view.controller.available ? qsTr("Dwell clicking is off") : (view.modes.find(mode => mode.id === view.controller.selected)?.label ?? qsTr("No click selected"))
                            }
                        }
                        PC.Label {
                            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                            text: view.controller.available ? qsTr("Choose the next click, then hold the pointer still over your target. Select the same mode again to clear it.") : qsTr("Enable dwell clicking in Accessibility settings to click by holding the pointer still.")
                        }
                    }
                }
                Details.DetailSurface {
                    objectName: "dwellModesCard"
                    Layout.fillWidth: true; radius: 22; implicitHeight: modeBody.implicitHeight + 28
                    ColumnLayout {
                        id: modeBody; anchors.fill: parent; anchors.margins: 14; spacing: 12
                        PC.Label { text: qsTr("Next click"); font.weight: Font.DemiBold }
                        GridLayout {
                            id: grid; Layout.fillWidth: true; columns: width < 300 ? 1 : 2
                            columnSpacing: 8; rowSpacing: 8; uniformCellWidths: true
                            Repeater {
                                model: view.modes
                                delegate: DwellActionButton {
                                    required property var modelData
                                    objectName: "dwellMode:" + modelData.id
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; Layout.minimumWidth: 0
                                    Layout.columnSpan: modelData.id === 10 ? grid.columns : 1
                                    implicitHeight: Math.max(46,implicitContentHeight + 16)
                                    text: modelData.label; icon.name: modelData.icon
                                    enabled: view.controller.available; checked: view.controller.selected === modelData.id
                                    onClicked: view.controller.select(modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer; objectName: "dwellFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        visible: view.controller.settingsAllowed
        width: Math.min(parent.width,settings.implicitWidth + 16)
        implicitHeight: settings.implicitHeight + 8; radius: height / 2; elevated: true
        DwellActionButton { id: settings; objectName: "dwellSettings"; anchors.centerIn: parent; flat: true; text: qsTr("Accessibility settings"); icon.name: "configure-symbolic"; onClicked: view.controller.settings() }
    }
}
