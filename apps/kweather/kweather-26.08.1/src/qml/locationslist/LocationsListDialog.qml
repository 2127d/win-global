// Copyright 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

LocationDialog {
    id: root

    title: i18n("Locations")

    footer: Control {
        padding: 16
        contentItem: RowLayout {
            Item { Layout.fillWidth: true }
            Button {
                id: addButton
                objectName: "weatherManageAddLocation"
                text: i18n("Add Location")
                Accessible.name: text
                implicitHeight: 44
                implicitWidth: Math.max(160, addContent.implicitWidth + 32)
                padding: 12
                leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
                hoverEnabled: true
                onClicked: {
                    root.close();
                    applicationWindow().openAddLocation();
                }
                background: Rectangle {
                    radius: 22
                    color: Qt.tint(Kirigami.Theme.backgroundColor,
                        Qt.alpha(Kirigami.Theme.textColor, addButton.down ? 0.08 : addButton.hovered ? 0.04 : 0))
                    border.width: addButton.activeFocus || addButton.down ? 2 : 1
                    border.color: addButton.activeFocus || addButton.down ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.15)
                    Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                }
                contentItem: RowLayout {
                    id: addContent
                    spacing: 8
                    Kirigami.Icon {
                        source: "list-add"
                        isMask: true
                        implicitWidth: 18; implicitHeight: 18
                        color: Kirigami.Theme.textColor
                    }
                    Label {
                        text: addButton.text
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    LocationsListView {
        onCloseRequested: root.close()
    }
}
