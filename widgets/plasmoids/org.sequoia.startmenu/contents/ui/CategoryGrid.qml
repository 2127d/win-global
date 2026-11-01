// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.components as QQC2
import org.kde.kirigami as Kirigami

Grid {
    id: categories
    required property var menu
    property bool populated: true
    columns: Math.max(1, Math.floor((width + spacing) / (130 + spacing)))
    spacing: 16
    Repeater {
        model: categories.populated ? categories.menu.backend.categories : null
        delegate: QQC2.AbstractButton {
            id: card
            objectName: "startCategoryCard"
            required property int index
            required property var model
            readonly property var apps: categories.menu.backend.categories.modelForRow(index)
            width: (categories.width - categories.spacing * (categories.columns - 1)) / categories.columns
            height: width
            padding: 0
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus
            text: model.display || ""
            Accessible.name: text
            onClicked: categories.menu.showCategory(apps, text)
            background: MenuSurface {
                hovered: card.hovered
                pressed: card.down
                focused: card.visualFocus
                panel: true
                radius: 22
                elevated: true
                motionDuration: categories.menu.feedbackDuration
            }
            contentItem: Item {
                Grid {
                    y: Math.max(12, (parent.height - 32 - height) / 2)
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
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 16
                    text: card.text
                    color: categories.menu.textColor
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
