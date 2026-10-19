// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.extras as PE
import org.kde.plasma.components as PC
import "../details" as Details

ColumnLayout {
    id: root
    property var contactModel
    signal searchChanged(string query)
    signal openContact(int itemId)
    spacing: 10
    function focusSearch() { search.forceActiveFocus(); search.selectAll(); }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_F && event.modifiers & Qt.ControlModifier) {
            focusSearch(); event.accepted = true;
        } else if (event.key === Qt.Key_Down && list.count > 0) {
            list.currentIndex = Math.max(0, list.currentIndex);
            list.forceActiveFocus(); event.accepted = true;
        }
    }
    PE.SearchField {
        id: search
        objectName: "contactsSearch"
        Layout.fillWidth: true
        implicitHeight: Math.max(40, contentHeight + 16)
        topPadding: 8
        bottomPadding: 8
        leftPadding: 36
        rightPadding: 36
        placeholderText: i18n("Search contacts")
        Accessible.name: placeholderText
        background: Details.DetailSurface { radius: 13; focused: search.activeFocus }
        onTextChanged: { list.currentIndex = -1; root.searchChanged(text); }
        Keys.onDownPressed: {
            if (list.count > 0) { list.currentIndex = 0; list.forceActiveFocus(); }
        }
    }
    Details.DetailSurface {
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: 18
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 0
            PC.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                padding: 0
                PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
                PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
                contentItem: ListView {
                    id: list
                    objectName: "contactsList"
                    model: root.contactModel
                    currentIndex: -1
                    clip: true
                    reuseItems: true
                    spacing: 3
                    boundsBehavior: Flickable.StopAtBounds
                    keyNavigationEnabled: true
                    activeFocusOnTab: true
                    highlightMoveDuration: Kirigami.Units.shortDuration
                    section.property: "display"
                    section.criteria: ViewSection.FirstCharacter
                    section.delegate: Item {
                        required property string section
                        width: ListView.view.width
                        height: label.implicitHeight + 18
                        PC.Label {
                            id: label
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: section.trim().length ? section.toLocaleUpperCase() : "#"
                            font.weight: Font.DemiBold
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            color: Kirigami.Theme.disabledTextColor
                            textFormat: Text.PlainText
                        }
                    }
                    delegate: PC.ItemDelegate {
                        id: row
                        required property var model
                        required property int index
                        width: ListView.view.width
                        implicitHeight: Math.max(60, implicitContentHeight + 18)
                        leftPadding: 10
                        rightPadding: 12
                        topPadding: 9
                        bottomPadding: 9
                        text: model.display?.trim() || i18n("No Name")
                        highlighted: ListView.isCurrentItem && list.activeFocus
                        onClicked: { list.currentIndex = index; root.openContact(model.itemId); }
                        background: Rectangle {
                            radius: 12
                            color: Qt.alpha(Kirigami.Theme.highlightColor, row.down ? 0.17 : row.highlighted ? 0.13 : row.hovered ? 0.07 : 0)
                            border.width: row.activeFocus ? 2 : 0
                            border.color: Kirigami.Theme.highlightColor
                            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            ContactAvatar {
                                name: row.text
                                photo: row.model.addressee?.photo
                                    ? (row.model.addressee.photo.url.toString().length ? row.model.addressee.photo.url : "image://avatar/" + row.model.itemId) : ""
                            }
                            PC.Label {
                                Layout.fillWidth: true
                                text: row.text
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                font.weight: Font.Medium
                                Accessible.ignored: true
                            }
                            Kirigami.Icon {
                                source: row.mirrored ? "go-previous-symbolic" : "go-next-symbolic"
                                implicitWidth: 14
                                implicitHeight: 14
                                opacity: row.hovered || row.highlighted ? 0.8 : 0.3
                                Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
                                Accessible.ignored: true
                            }
                        }
                    }
                    Keys.onReturnPressed: if (currentItem) root.openContact(currentItem.model.itemId)
                    Keys.onEnterPressed: if (currentItem) root.openContact(currentItem.model.itemId)
                    Keys.onUpPressed: {
                        if (currentIndex <= 0) { currentIndex = -1; root.focusSearch(); }
                        else decrementCurrentIndex();
                    }
                }
            }
            PC.Label {
                Layout.fillWidth: true
                topPadding: 8
                bottomPadding: 4
                horizontalAlignment: Text.AlignHCenter
                text: i18np("%1 contact", "%1 contacts", list.count)
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                color: Kirigami.Theme.disabledTextColor
            }
        }
        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 48
            visible: list.count === 0
            spacing: 10
            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 40; implicitHeight: 40
                source: search.text.length ? "search" : "view-pim-contacts"
                opacity: 0.45
            }
            PC.Label {
                Layout.fillWidth: true
                text: search.text.length ? i18n("No matching contacts") : i18n("No contacts yet")
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
            PC.Label {
                Layout.fillWidth: true
                text: search.text.length ? i18n("Try a different name.") : i18n("Contacts from your address books will appear here.")
                color: Kirigami.Theme.disabledTextColor
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}
