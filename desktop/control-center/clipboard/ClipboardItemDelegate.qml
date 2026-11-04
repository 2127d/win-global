/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../../details" as Details

PC.ItemDelegate {
    id: menuItem
    objectName: "clipboardCard:" + uuid
    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
    height: body.implicitHeight + 24
    leftPadding: 12; rightPadding: 12; topPadding: 12; bottomPadding: 12
    required property var listMargins
    required property var model
    required property int index
    required property var decoration
    required property size imageSize
    required property var uuid
    required property int type
    property bool keyboardActivated: false
    readonly property alias dragHandler: dragHandler
    property alias mainItem: preview.contentItem
    readonly property var clipboardMenu: ListView.view.clipboardMenu
    readonly property string kind: type === 4 ? qsTr("Image") : type === 8 ? qsTr("Files") : qsTr("Text")
    signal itemSelected()
    signal remove()
    signal edit()
    signal barcode()
    signal triggerAction()
    onClicked: itemSelected()
    onItemSelected: clipboardMenu.itemSelected(uuid)
    onRemove: clipboardMenu.remove(uuid)
    onEdit: clipboardMenu.edit(model)
    onBarcode: clipboardMenu.barcode(model.display)
    onTriggerAction: clipboardMenu.triggerAction(uuid)
    function toggleStar() {
        clipboardMenu.hoverBlock.reset();
        clipboardMenu.keyboardActivated = true;
        model.starred = !model.starred;
    }
    Accessible.description: model.starred ? qsTr("%1, starred. Activate to copy.").arg(kind) : qsTr("%1. Activate to copy.").arg(kind)
    Accessible.onPressAction: itemSelected()
    Keys.onEnterPressed: event => Keys.returnPressed(event)
    Keys.onReturnPressed: itemSelected()
    Keys.onDeletePressed: remove()
    KeyNavigation.right: starButton
    onHoveredChanged: if (hovered) ListView.view.currentIndex = index
    ListView.onPooled: actionMenu.close()
    background: Details.DetailSurface {
        objectName: "sequoiaClipboardCard"
        radius: 20
        hovered: menuItem.hovered
        focused: menuItem.visualFocus || (menuItem.ListView.isCurrentItem && menuItem.keyboardActivated && menuItem.ListView.view.activeFocus)
    }
    contentItem: ColumnLayout {
        id: body
        spacing: 10
        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Rectangle {
                Layout.preferredWidth: 28; Layout.preferredHeight: 28
                radius: 10; color: Qt.alpha(Kirigami.Theme.highlightColor,0.12)
                Kirigami.Icon {
                    anchors.centerIn: parent; width: 16; height: 16
                    source: menuItem.type === 4 ? "image-x-generic-symbolic" : menuItem.type === 8 ? "folder-symbolic" : "text-x-generic-symbolic"
                    isMask: true; color: Kirigami.Theme.textColor
                }
            }
            PC.Label {
                Layout.fillWidth: true
                text: menuItem.type === 4 ? qsTr("Image · %1 × %2").arg(menuItem.imageSize.width).arg(menuItem.imageSize.height) : menuItem.kind
                elide: Text.ElideRight; color: Kirigami.Theme.disabledTextColor
                font: Kirigami.Theme.smallFont
            }
            ClipboardActionButton {
                id: starButton
                objectName: "clipboardStar:" + menuItem.uuid
                flat: true; display: PC.AbstractButton.IconOnly
                text: menuItem.model.starred ? qsTr("Remove star") : qsTr("Star item")
                icon.name: menuItem.model.starred ? "starred-symbolic" : "non-starred-symbolic"
                onClicked: menuItem.toggleStar()
                KeyNavigation.left: menuItem; KeyNavigation.right: copyButton
                PC.ToolTip { text: parent.text; visible: parent.hovered; delay: Kirigami.Units.toolTipDelay }
            }
            ClipboardActionButton {
                id: copyButton
                objectName: "clipboardCopy:" + menuItem.uuid
                flat: true; display: PC.AbstractButton.IconOnly
                text: qsTr("Copy again"); icon.name: "edit-copy-symbolic"
                onClicked: menuItem.itemSelected()
                KeyNavigation.left: starButton; KeyNavigation.right: deleteButton
                PC.ToolTip { text: parent.text; visible: parent.hovered; delay: Kirigami.Units.toolTipDelay }
            }
            ClipboardActionButton {
                id: deleteButton
                objectName: "clipboardRemove:" + menuItem.uuid
                flat: true; display: PC.AbstractButton.IconOnly
                text: qsTr("Remove from history"); icon.name: "edit-delete-symbolic"
                onClicked: menuItem.remove()
                KeyNavigation.left: copyButton; KeyNavigation.right: moreButton
                PC.ToolTip { text: parent.text; visible: parent.hovered; delay: Kirigami.Units.toolTipDelay }
            }
            ClipboardActionButton {
                id: moreButton
                objectName: "clipboardMore:" + menuItem.uuid
                flat: true; display: PC.AbstractButton.IconOnly
                text: qsTr("More actions"); icon.name: "view-more-symbolic"
                onClicked: actionMenu.popup(moreButton,0,moreButton.height)
                KeyNavigation.left: deleteButton
                PC.ToolTip { text: parent.text; visible: parent.hovered; delay: Kirigami.Units.toolTipDelay }
            }
        }
        QQC.Control {
            id: preview
            Layout.fillWidth: true
            implicitHeight: contentItem?.implicitHeight || 0
            padding: 0
            clip: true
            DragHandler { id: dragHandler; target: null }
        }
    }
    ClipboardActionMenu {
        id: actionMenu
        objectName: "clipboardActions:" + menuItem.uuid
        ClipboardActionMenuItem { objectName: "clipboardEdit:" + menuItem.uuid; text: qsTr("Edit contents"); icon.name: "document-edit-symbolic"; visible: menuItem.type === 2; height: visible ? implicitHeight : 0; onTriggered: menuItem.edit() }
        ClipboardActionMenuItem { objectName: "clipboardQr:" + menuItem.uuid; text: qsTr("Show QR code"); icon.name: "view-barcode-qr"; visible: menuItem.type === 2; height: visible ? implicitHeight : 0; onTriggered: menuItem.barcode() }
        ClipboardActionMenuItem { objectName: "clipboardInvoke:" + menuItem.uuid; text: qsTr("Invoke action"); icon.name: "system-run"; onTriggered: menuItem.triggerAction() }
    }
}
