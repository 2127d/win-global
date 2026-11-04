/*
    SPDX-FileCopyrightText: 2021 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.clipboard as Private
import "../../details" as Details

ColumnLayout {
    id: editPage
    objectName: "clipboardEditPage"
    spacing: 12
    required property PlasmaExtras.Representation dialogItem
    required property Private.HistoryModel historyModel
    required property var modelData
    property alias text: textArea.text
    property Item header: Item { visible: false }
    Keys.onEscapePressed: T.StackView.view.popCurrentItem()
    function saveAndExit() {
        modelData.display = text;
        historyModel.moveToTop(modelData.uuid);
        T.StackView.view.popCurrentItem();
        T.StackView.view.initialItem.view.currentIndex = 0;
    }
    T.StackView.onStatusChanged: {
        if (editPage.T.StackView.status === T.StackView.Active) {
            textArea.forceActiveFocus(Qt.ActiveWindowFocusReason);
            textArea.cursorPosition = textArea.text.length;
        }
    }
    Shortcut { enabled: textArea.canUndo; sequences: [StandardKey.Save]; onActivated: editPage.saveAndExit() }
    Details.DetailSurface {
        Layout.fillWidth: true; Layout.fillHeight: true
        Layout.margins: 6; radius: 24
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 12
            PC.Label { text: qsTr("Edit text"); font.weight: Font.DemiBold }
            PC.Label { Layout.fillWidth: true; text: qsTr("Update this copy before using it again."); wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
            PC.ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true
                PC.TextArea {
                    id: textArea
                    objectName: "clipboardEditor"
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.PlainText
                    text: editPage.modelData?.display ?? ""
                    background: null
                    Accessible.name: qsTr("Clipboard text")
                    KeyNavigation.up: editPage.dialogItem.KeyNavigation.up
                    Keys.onPressed: event => {
                        if (textArea.canUndo && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                            editPage.saveAndExit(); event.accepted = true;
                        } else event.accepted = false;
                    }
                }
            }
            PC.Label { Layout.fillWidth: true; text: qsTr("Enter to save · Shift+Enter for a new line"); font: Kirigami.Theme.smallFont; color: Kirigami.Theme.disabledTextColor; wrapMode: Text.Wrap }
        }
    }
    Details.DetailSurface {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: actions.implicitWidth + 16; implicitHeight: actions.implicitHeight + 8
        radius: height / 2; elevated: true
        RowLayout {
            id: actions; anchors.centerIn: parent; spacing: 4
            ClipboardActionButton { objectName: "clipboardEditCancel"; flat: true; text: qsTr("Cancel"); icon.name: "dialog-cancel"; onClicked: editPage.T.StackView.view.popCurrentItem(); KeyNavigation.up: textArea }
            ClipboardActionButton { objectName: "clipboardEditSave"; enabled: textArea.canUndo; flat: true; text: qsTr("Save"); icon.name: "document-save"; onClicked: editPage.saveAndExit(); KeyNavigation.up: textArea }
        }
    }
}
