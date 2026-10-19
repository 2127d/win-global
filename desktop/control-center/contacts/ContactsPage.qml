// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.merkuro.contact
import org.kde.kitemmodels

ContactsView {
    property string title: i18n("Contacts")
    contactModel: KSortFilterProxyModel {
        id: contacts
        filterCaseSensitivity: Qt.CaseInsensitive
        sourceModel: ContactsModel {}
    }
    onSearchChanged: query => contacts.setFilterFixedString(query)
    onOpenContact: itemId => stack.push(Qt.resolvedUrl("ContactPage.qml"), {itemId: itemId})
}
