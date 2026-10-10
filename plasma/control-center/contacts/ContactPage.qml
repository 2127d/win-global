// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.merkuro.contact

ContactDetails {
    id: root
    property int itemId
    property string title: i18n("Contact")
    contact: AddresseeWrapper { addresseeItem: ContactManager.getItem(root.itemId) }
    onActivate: (scheme, value) => Qt.openUrlExternally(scheme + value)
    onShowCode: stack.push(Qt.resolvedUrl("QrCodePage.qml"), { qrCodeData: contact.qrCodeData() })
}
