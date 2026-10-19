// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PE
import org.kde.merkuro.contact

PlasmoidItem {
    id: contactApplet
    objectName: "sequoiaContactsApplet"
    // Akonadi can wait synchronously for its service during login. Never
    // construct the contacts database model while Plasma preloads applets.
    preloadFullRepresentation: false
    Plasmoid.icon: "im-user-symbolic"
    switchWidth: Kirigami.Units.gridUnit * 5
    switchHeight: Kirigami.Units.gridUnit * 5
    Component.onCompleted: AvatarImageProvider.init()
    readonly property var backAction: Kirigami.Action {
        enabled: (contactApplet.fullRepresentationItem?.navigationDepth ?? 1) > 1
        onTriggered: contactApplet.fullRepresentationItem.goBack()
    }
    fullRepresentation: PE.Representation {
        objectName: "sequoiaContactsPage"
        Layout.minimumWidth: Kirigami.Units.gridUnit * 5
        Layout.minimumHeight: Kirigami.Units.gridUnit * 5
        padding: 0
        background: null
        collapseMarginsHint: true
        focus: true
        readonly property int navigationDepth: stack.depth
        function goBack() { if (stack.depth > 1) stack.pop(); }
        readonly property string itemTitle: stack.currentItem?.title ?? i18n("Contacts")
        onItemTitleChanged: contactApplet.Plasmoid.title = itemTitle
        Keys.forwardTo: [stack.currentItem]
        Keys.onEscapePressed: event => {
            if (stack.depth > 1) { goBack(); event.accepted = true; }
            else event.accepted = false;
        }
        QQC2.StackView {
            id: stack
            objectName: "contactsStack"
            anchors.fill: parent
            clip: true
            initialItem: null
            Component { id: contactsPage; ContactsPage {} }
            function loadContacts() {
                if (contactApplet.expanded && depth === 0) push(contactsPage);
            }
            Component.onCompleted: loadContacts()
            Connections {
                target: contactApplet
                function onExpandedChanged() { stack.loadContacts(); }
            }
            pushEnter: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.shortDuration }
            }
            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.shortDuration }
            }
            popEnter: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.shortDuration }
            }
            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.shortDuration }
            }
        }
    }
}
