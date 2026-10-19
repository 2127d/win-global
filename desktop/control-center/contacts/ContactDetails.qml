// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

PC.ScrollView {
    id: root
    required property var contact
    signal showCode()
    signal activate(string scheme, string value)
    padding: 0
    PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
    PC.ScrollBar.vertical.policy: PC.ScrollBar.AlwaysOff
    contentWidth: availableWidth
    contentHeight: column.implicitHeight
    clip: true
    ColumnLayout {
        id: column
        width: root.availableWidth
        spacing: 10
        PC.Control {
            Layout.fillWidth: true
            padding: 20
            background: Details.DetailSurface { radius: 18 }
            contentItem: ColumnLayout {
                spacing: 12
                ContactAvatar {
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: 72
                    implicitHeight: 72
                    name: root.contact.formattedName || i18n("No Name")
                    photo: root.contact.photoUrl || ""
                }
                PC.Label {
                    Layout.fillWidth: true
                    text: root.contact.formattedName || i18n("No Name")
                    textFormat: Text.PlainText
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.45
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }
                PC.Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: root.contact.nickName || ""
                    textFormat: Text.PlainText
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    ContactAction {
                        visible: !!root.contact.preferredEmail
                        text: i18n("Email")
                        icon.name: "mail-message-new"
                        onClicked: root.activate("mailto:", root.contact.preferredEmail)
                    }
                    ContactAction {
                        objectName: "contactShare"
                        text: i18n("QR code")
                        icon.name: "view-barcode-qr"
                        onClicked: root.showCode()
                    }
                }
            }
        }
        ContactFields {
            objectName: "contactPhones"
            Layout.fillWidth: true
            title: i18n("Phone")
            kind: "phone"
            fieldModel: root.contact.phoneModel
            visible: count > 0
            onActivate: (scheme, value) => root.activate(scheme, value)
        }
        ContactFields {
            objectName: "contactEmails"
            Layout.fillWidth: true
            title: i18n("Email")
            kind: "email"
            fieldModel: root.contact.emailModel
            visible: count > 0
            onActivate: (scheme, value) => root.activate(scheme, value)
        }
        ContactFields {
            objectName: "contactAddresses"
            Layout.fillWidth: true
            title: i18n("Address")
            kind: "address"
            fieldModel: root.contact.addressesModel
            visible: count > 0
        }
        PC.Control {
            id: birthdayCard
            Layout.fillWidth: true
            readonly property var birthday: root.contact.birthday
            readonly property bool validDate: birthday instanceof Date && !isNaN(birthday.getTime())
            visible: validDate
            padding: 16
            background: Details.DetailSurface { radius: 18 }
            contentItem: ColumnLayout {
                spacing: 8
                PC.Label { text: i18n("Birthday"); font.weight: Font.DemiBold }
                PC.Label {
                    Layout.fillWidth: true
                    text: birthdayCard.validDate ? Qt.formatDate(birthdayCard.birthday,
                        birthdayCard.birthday.getFullYear() === 0 ? "MMMM d" : Qt.DefaultLocaleLongDate) : ""
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
