// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.plasma.networkmanagement as PlasmaNM
import "../details" as Details

Details.DetailExpandable {
    id: networkRow
    // Keep KDE's action and keyboard handling; use the shared network pill surface.
    NetworkButtonSurface {
        id: actionSurface
        control: networkRow.defaultActionControl
    }
    Binding { target: networkRow.defaultActionControl; property: "background"; value: actionSurface }
    Binding { target: networkRow.defaultActionControl; property: "flat"; value: false }
    Binding { target: networkRow.defaultActionControl; property: "leftPadding"; value: 12 }
    Binding { target: networkRow.defaultActionControl; property: "rightPadding"; value: 12 }
    Binding { target: networkRow.defaultActionControl; property: "topPadding"; value: 7 }
    Binding { target: networkRow.defaultActionControl; property: "bottomPadding"; value: 7 }
    Binding { target: networkRow.defaultActionControl.Layout; property: "rightMargin"; value: Qt.application.layoutDirection === Qt.RightToLeft ? 0 : 8 }
    Binding { target: networkRow.defaultActionControl.Layout; property: "leftMargin"; value: Qt.application.layoutDirection === Qt.RightToLeft ? 8 : 0 }

    Binding { target: networkRow.expandToggleControl.Layout; property: "rightMargin"; value: Qt.application.layoutDirection === Qt.RightToLeft ? 0 : 8 }
    Binding { target: networkRow.expandToggleControl.Layout; property: "leftMargin"; value: Qt.application.layoutDirection === Qt.RightToLeft ? 8 : 0 }
    contextualActionsHorizontal: true
    contextualActionSpacing: 8
    contextualActionDelegate: Component {
        NetworkActionButton {
            required property int index
            required property T.Action modelData
            objectName: "networkContextAction:" + index
            Layout.fillWidth: false
            Layout.minimumWidth: 0
            Layout.maximumWidth: implicitWidth
            text: modelData.text
            icon.name: modelData.icon.name
            enabled: modelData.enabled
            KeyNavigation.up: index > 0 ? networkRow.contextualActionAt(index - 1) : networkRow.expandToggleControl
            Keys.onDownPressed: event => {
                const next = networkRow.contextualActionAt(index + 1);
                if (next) next.forceActiveFocus(Qt.TabFocusReason);
                else {
                    networkRow.ListView.view.incrementCurrentIndex();
                    networkRow.ListView.view.currentItem?.forceActiveFocus(Qt.TabFocusReason);
                }
                event.accepted = true;
            }
            onClicked: modelData.trigger()
        }
    }

    onVisibleChanged: if (!visible) closeContextualMenu()
    Connections {
        target: networkRow.Window.window
        function onVisibleChanged() { if (!networkRow.Window.window.visible) networkRow.closeContextualMenu(); }
    }
    Connections {
        target: typeof mainWindow !== "undefined" ? mainWindow : null
        function onExpandedChanged() { if (!mainWindow.expanded) networkRow.closeContextualMenu(); }
    }

    property int connectionType: PlasmaNM.Enums.UnknownConnectionType
    property string connectionIcon: "network-wireless-symbolic"
    readonly property bool bluetoothNetwork: connectionType === PlasmaNM.Enums.Bluetooth
    // ListView positions delegates at x=0 during layout. Anchor their centers
    // so the narrower grouped rows retain equal left and right insets.
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    icon: bluetoothNetwork ? "network-wireless-symbolic" : connectionIcon
    iconEmblem: bluetoothNetwork ? "preferences-system-bluetooth-symbolic" : ""
}
