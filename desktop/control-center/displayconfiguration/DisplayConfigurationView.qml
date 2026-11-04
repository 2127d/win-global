// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    objectName: "displayConfigurationView"
    required property var controller
    readonly property real naturalHeight: content.implicitHeight + (footer.visible ? footer.implicitHeight + 12 : 0) + 8
    // KScreen OsdAction values: mirror/extend together, then single-output modes.
    // Keep new backend actions visible even if their enum is not known here.
    readonly property var orderedLayouts: Array.from(controller.layouts).sort((a, b) => {
        const order = [3, 4, 5, 2, 1];
        const rank = action => order.includes(action) ? order.indexOf(action) : order.length;
        return rank(a.action) - rank(b.action);
    })
    readonly property bool hasMirror: orderedLayouts.some(preset => preset.action === 3)
    readonly property bool canArrange: controller.connectedCount > 1
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PC.ScrollView {
        id: scroll
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footer.visible ? footer.top : parent.bottom; bottomMargin: footer.visible ? 12 : 0 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true
            contentWidth: width; contentHeight: content.implicitHeight + 8
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content
                width: parent.width; spacing: 12
                Details.DetailSurface {
                    objectName: "configurationLayoutCard"
                    Layout.fillWidth: true
                    radius: 22; implicitHeight: layoutBody.implicitHeight + 28
                    ColumnLayout {
                        id: layoutBody
                        anchors.fill: parent; anchors.margins: 14; spacing: 12
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Kirigami.Icon { Layout.preferredWidth: 24; Layout.preferredHeight: 24; source: "video-display-symbolic" }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                PC.Label { Layout.fillWidth: true; text: qsTr("Screen layout"); font.weight: Font.DemiBold; wrapMode: Text.Wrap }
                                PC.Label {
                                    Layout.fillWidth: true; color: Kirigami.Theme.disabledTextColor; font: Kirigami.Theme.smallFont; wrapMode: Text.Wrap
                                    text: view.controller.connectedCount === 1 ? qsTr("1 display connected") : qsTr("%1 displays connected").arg(view.controller.connectedCount)
                                }
                            }
                        }
                        PC.Label {
                            Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText
                            color: Kirigami.Theme.disabledTextColor
                            text: view.canArrange ? qsTr("Choose how your displays work together.") : qsTr("Connect another display to change the screen layout.")
                        }
                        GridLayout {
                            id: presets
                            objectName: "configurationPresetGrid"
                            Layout.fillWidth: true
                            columns: width < 320 ? 1 : 2; columnSpacing: 8; rowSpacing: 8
                            uniformCellWidths: true
                            Repeater {
                                model: view.orderedLayouts
                                delegate: LayoutPresetButton {
                                    required property var modelData
                                    required property int index
                                    objectName: "configurationPreset:" + modelData.action
                                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1
                                    Layout.columnSpan: presets.columns === 2 && (modelData.action === 3 || (!view.hasMirror && index === view.orderedLayouts.length - 1 && view.orderedLayouts.length % 2)) ? 2 : 1
                                    preset: modelData; enabled: view.canArrange
                                    onClicked: view.controller.applyLayout(modelData.action)
                                }
                            }
                        }
                    }
                }
                Details.DetailSurface {
                    objectName: "configurationPresentationCard"
                    Layout.fillWidth: true
                    radius: 22; implicitHeight: presentationBody.implicitHeight + 28
                    ColumnLayout {
                        id: presentationBody
                        anchors.fill: parent; anchors.margins: 14; spacing: 10
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Kirigami.Icon { Layout.preferredWidth: 24; Layout.preferredHeight: 24; source: "view-presentation-symbolic" }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                PC.Label { Layout.fillWidth: true; text: qsTr("Presentation Mode"); wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                                PC.Label {
                                    Layout.fillWidth: true; font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap
                                    text: view.controller.presentationBusy ? qsTr("Updating…") : view.controller.presentationEnabled ? qsTr("On") : qsTr("Off")
                                }
                            }
                            PC.Switch {
                                objectName: "configurationPresentationSwitch"
                                Accessible.name: qsTr("Presentation Mode")
                                checked: view.controller.presentationEnabled
                                enabled: view.controller.powerAvailable && !view.controller.presentationBusy
                                onToggled: {
                                    view.controller.setPresentationMode(checked);
                                    checked = Qt.binding(() => view.controller.presentationEnabled);
                                }
                            }
                        }
                        PC.Label {
                            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                            text: view.controller.powerAvailable ? qsTr("Keep the screen and computer from turning off automatically.") : qsTr("Power management is currently unavailable.")
                        }
                        PC.Label {
                            objectName: "configurationError"
                            Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText
                            visible: text.length > 0; text: view.controller.errorMessage
                            color: Kirigami.Theme.negativeTextColor
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Qt.alpha(Kirigami.Theme.textColor, 0.12); visible: view.controller.inhibitions.length > 0 }
                        PC.Label {
                            Layout.fillWidth: true; wrapMode: Text.Wrap; font.pointSize: Kirigami.Theme.smallFont.pointSize; font.weight: Font.DemiBold
                            text: qsTr("Apps keeping the screen awake"); visible: view.controller.inhibitions.length > 0
                        }
                        Repeater {
                            model: view.controller.inhibitions
                            delegate: RowLayout {
                                required property var modelData
                                objectName: "configurationInhibition"
                                Layout.fillWidth: true; spacing: 10
                                Kirigami.Icon { Layout.preferredWidth: 22; Layout.preferredHeight: 22; source: modelData.Icon || "application-x-executable" }
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 2
                                    PC.Label { Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText; text: modelData.Name || qsTr("Application"); font.weight: Font.Medium }
                                    PC.Label {
                                        Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText
                                        text: modelData.Reason || qsTr("Keeping the screen awake"); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer
        objectName: "configurationFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, settings.implicitWidth + 16)
        implicitHeight: settings.implicitHeight + 8
        radius: height / 2; elevated: true
        visible: view.controller.settingsAllowed
        ConfigurationActionButton {
            id: settings
            objectName: "configurationSettings"
            anchors.centerIn: parent
            flat: true; text: qsTr("Display settings"); icon.name: "configure-symbolic"
            onClicked: view.controller.settings()
        }
    }
}
