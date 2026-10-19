// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

PlasmoidItem {
    preferredRepresentation: fullRepresentation
    Layout.minimumWidth: 17
    Layout.maximumWidth: 17
    Layout.fillHeight: true
    fullRepresentation: Item {
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: Math.min(32, parent.height * 0.6)
            color: Kirigami.Theme.textColor
            opacity: 0.22
        }
    }
}
