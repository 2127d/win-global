// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

PC.ItemDelegate {
    id: card
    required property var note
    required property var controller
    property bool replying: false
    objectName: "connectNotification"
    implicitHeight: body.implicitHeight + 24
    padding: 12
    onClicked: checked = !checked
    background: Details.DetailSurface { radius: 16; hovered: card.hovered; focused: card.visualFocus }
    function sendReply() {
        if (!controller.reachable || !note.repliable || replyText.text.length === 0) return;
        controller.reply(note, replyText.text);
        replyText.text = ""; replying = false;
    }
    contentItem: ColumnLayout {
        id: body; spacing: 8
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Kirigami.Icon { Layout.preferredWidth: 22; Layout.preferredHeight: 22; source: card.note.appIcon || "notifications-symbolic" }
            PC.Label { Layout.fillWidth: true; text: card.note.appName; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.weight: Font.DemiBold }
            ConnectActionButton {
                objectName: "connectReply"
                icon.name: "mail-reply-sender-symbolic"; text: qsTr("Reply"); display: PC.AbstractButton.IconOnly; flat: true
                visible: card.note.repliable; enabled: card.controller.reachable && !card.replying
                onClicked: { card.replying = true; replyText.forceActiveFocus(); }
                PC.ToolTip { text: parent.text }
            }
            ConnectActionButton {
                objectName: "connectDismiss"
                icon.name: "window-close-symbolic"; text: qsTr("Dismiss"); display: PC.AbstractButton.IconOnly; flat: true
                visible: card.controller.dismissable; enabled: card.controller.reachable && card.note.dismissable
                onClicked: card.controller.dismiss(card.note)
                PC.ToolTip { text: parent.text }
            }
        }
        PC.Label {
            Layout.fillWidth: true; text: card.note.title; visible: text.length > 0
            textFormat: Text.PlainText; font.weight: Font.Medium; wrapMode: Text.Wrap
            maximumLineCount: card.checked ? 0 : 2; elide: card.checked ? Text.ElideNone : Text.ElideRight
        }
        PC.Label {
            Layout.fillWidth: true; text: card.note.notitext; visible: text.length > 0
            textFormat: Text.PlainText; wrapMode: Text.Wrap
            maximumLineCount: card.checked ? 0 : 2; elide: card.checked ? Text.ElideNone : Text.ElideRight
        }
        ColumnLayout {
            visible: card.replying; Layout.fillWidth: true; spacing: 6
            PC.TextArea {
                id: replyText; objectName: "connectReplyText"
                Layout.fillWidth: true; wrapMode: TextEdit.Wrap
                placeholderText: qsTr("Reply to %1…").arg(card.note.appName)
                Keys.onPressed: event => {
                    if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) { card.sendReply(); event.accepted = true; }
                    if (event.key === Qt.Key_Escape) { text = ""; card.replying = false; event.accepted = true; }
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                ConnectActionButton { objectName: "connectCancelReply"; text: qsTr("Cancel"); onClicked: { replyText.text = ""; card.replying = false; } }
                ConnectActionButton { objectName: "connectSendReply"; text: qsTr("Send"); icon.name: "document-send-symbolic"; enabled: replyText.text.length > 0 && card.controller.reachable; onClicked: card.sendReply() }
            }
        }
    }
}
