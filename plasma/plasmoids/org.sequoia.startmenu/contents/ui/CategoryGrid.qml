// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.components as QQC2
import org.kde.kirigami as Kirigami

Grid {
    id: categories
    required property var menu
    property bool populated: true
    columns: Math.max(2, Math.floor(width / 130))
    spacing: 14
    Repeater {
        model: categories.populated ? categories.menu.backend.categories : null
        delegate: QQC2.AbstractButton {
            id: card
            required property int index
            required property var model
            readonly property var apps: categories.menu.backend.categories.modelForRow(index)
            width: (categories.width - categories.spacing * (categories.columns - 1)) / categories.columns
            height: 142
            hoverEnabled: true
            text: model.display || ""
            Accessible.name: text
            onClicked: categories.menu.showCategory(apps, text)
            background: Item {
                Rectangle {
                    width: parent.width; height: 112; radius: 9
                    color: card.hovered ? categories.menu.hoverColor : categories.menu.tileColor
                    border.width: card.activeFocus ? 2 : 1
                    border.color: card.activeFocus ? categories.menu.accentColor : categories.menu.borderColor
                }
            }
            contentItem: Item {
                Grid {
                    anchors.top: parent.top; anchors.topMargin: 18
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2; spacing: 18
                    Repeater {
                        model: card.apps
                        delegate: Loader {
                            id: iconSlot
                            required property int index
                            required property var model
                            active: index < 4
                            visible: active
                            width: active ? 28 : 0; height: active ? 28 : 0
                            sourceComponent: Kirigami.Icon {
                                objectName: "startCategoryIcon"
                                source: iconSlot.model.decoration || "application-x-executable"
                            }
                        }
                    }
                }
                Text {
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 7
                    width: parent.width
                    text: card.text
                    color: categories.menu.textColor
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
