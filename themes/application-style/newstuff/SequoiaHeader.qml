// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
import org.sequoia.components as S

ColumnLayout {
    id: header
    required property var page
    required property var stack
    spacing: 12
    S.Theme { id: controlTheme }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        S.CircleButton {
            objectName: "newStuffBack"
            theme: controlTheme
            visible: header.stack && header.stack.depth > 1
            label: qsTr("Back")
            icon.name: "go-previous"
            onClicked: Qt.callLater(() => header.stack.pop())
        }
        K.Heading {
            Layout.fillWidth: true
            level: 2
            font.pointSize: 15
            font.weight: Font.DemiBold
            text: header.page.title
            color: controlTheme.textColor
            elide: Text.ElideRight
        }
    }
    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 8
        Repeater {
            model: header.page.actions
            delegate: Loader {
                id: actionLoader
                required property var modelData
                required property int index
                visible: modelData.visible
                sourceComponent: modelData.displayComponent || actionButton
                Component {
                    id: actionButton
                    S.ActionPill {
                        id: button
                        objectName: "newStuffAction" + actionLoader.index
                        readonly property bool menuOpen: menu.visible
                        theme: controlTheme
                        action: actionLoader.modelData
                        // Menu actions have no native handler; their children
                        // remain the original actions, including ActionGroups.
                        onClicked: {
                            if (actionLoader.modelData.children.length > 0) {
                                if (menu.visible) menu.close(); else menu.open();
                            }
                        }
                        S.PopupMenu {
                            id: menu
                            theme: controlTheme
                            trigger: button
                            backdropItem: header.page
                            Instantiator {
                                model: actionLoader.modelData.children
                                delegate: S.MenuItem {
                                    required property var modelData
                                    theme: controlTheme
                                    action: modelData
                                }
                                onObjectAdded: (index, object) => menu.insertItem(index, object)
                                onObjectRemoved: (index, object) => menu.removeItem(object)
                            }
                        }
                    }
                }
            }
        }
    }
}
