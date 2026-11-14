// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import "../details" as Details

FocusScope {
    id: view
    objectName: "printersView"
    required property var controller
    required property Component printerDelegate
    readonly property real naturalHeight: search.implicitHeight + content.implicitHeight + footer.implicitHeight + 32
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    FontMetrics { id: searchMetrics; font: search.font }
    Details.DetailSearchField {
        id: search; objectName: "printerSearch"
        anchors { top: parent.top; left: parent.left; right: parent.right }
        implicitHeight: Math.max(40, searchMetrics.height + 20)
        topPadding: 10; bottomPadding: 10
        placeholderText: qsTr("Search printers…")
        onTextChanged: view.controller.search(text)
        background: Details.DetailSurface { radius: height / 2; focused: search.activeFocus }
    }
    PC.ScrollView {
        anchors { top: search.bottom; topMargin: 12; left: parent.left; right: parent.right; bottom: footer.top; bottomMargin: 12 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            contentWidth: width; contentHeight: content.implicitHeight + 8
            clip: true; layer.enabled: true; boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content; width: parent.width; spacing: 12
                Details.DetailSurface {
                    objectName: "printerError"
                    Layout.fillWidth: true; radius: 18; implicitHeight: errorLabel.implicitHeight + 24
                    visible: view.controller.error.length > 0 && !view.controller.serverUnavailable
                    PC.Label { id: errorLabel; anchors.fill: parent; anchors.margins: 12; text: view.controller.error; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: Kirigami.Theme.negativeTextColor }
                }
                Repeater { id: printers; model: view.controller.serverUnavailable ? null : view.controller.printersModel; delegate: view.printerDelegate }
                Details.DetailSurface {
                    objectName: "printerEmptyCard"
                    Layout.fillWidth: true; radius: 22; implicitHeight: emptyBody.implicitHeight + 32
                    visible: printers.count === 0
                    ColumnLayout {
                        id: emptyBody; anchors.fill: parent; anchors.margins: 16; spacing: 12
                        Kirigami.Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 40; Layout.preferredHeight: 40; source: view.controller.serverUnavailable ? "dialog-error-symbolic" : "printer-symbolic" }
                        PC.Label {
                            objectName: "printerEmptyTitle"
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; font.weight: Font.DemiBold; wrapMode: Text.Wrap
                            text: view.controller.serverUnavailable ? qsTr("Printing service unavailable") : search.text.length > 0 ? qsTr("No matching printers") : qsTr("No printers found")
                        }
                        PC.Label {
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; textFormat: Text.PlainText; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                            text: view.controller.serverUnavailable ? (view.controller.error || qsTr("Check the printing service, then try refreshing.")) : search.text.length > 0 ? qsTr("Try a different name or clear your search.") : qsTr("Configure a printer to see its status and print jobs here.")
                        }
                        PrinterActionButton { Layout.alignment: Qt.AlignHCenter; visible: search.text.length > 0 && !view.controller.serverUnavailable; text: qsTr("Clear search"); icon.name: "edit-clear-symbolic"; onClicked: search.clear() }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer; objectName: "printerFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, footerActions.implicitWidth + 16)
        implicitHeight: footerActions.implicitHeight + 8; radius: height / 2; elevated: true
        RowLayout {
            id: footerActions; anchors.centerIn: parent; spacing: 4
            PrinterActionButton { objectName: "printersSettings"; flat: true; visible: view.controller.settingsAllowed; text: qsTr("Printer settings"); icon.name: "configure-symbolic"; onClicked: view.controller.settings() }
            PrinterActionButton { objectName: "printersRefresh"; flat: true; text: qsTr("Refresh"); icon.name: "view-refresh-symbolic"; onClicked: view.controller.refresh() }
        }
    }
}
