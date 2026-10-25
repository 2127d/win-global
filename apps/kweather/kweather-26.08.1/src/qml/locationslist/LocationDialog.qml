// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kweather.components

Kirigami.Dialog {
    id: root
    property string closeButtonObjectName: "weatherCloseLocations"
    // Keep modal input blocking, with the shared visual scrim in Main.
    Overlay.modal: Item {}
    standardButtons: Kirigami.Dialog.NoButton
    parent: applicationWindow().overlay
    preferredHeight: 440
    preferredWidth: 480
    padding: 16
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    background: LocationSurface {
        radius: 24
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.88)
    }
    header: Control {
        padding: 12
        background: null
        contentItem: Item {
            implicitHeight: 32
            Label {
                objectName: "weatherLocationDialogTitle"
                anchors.fill: parent
                anchors.leftMargin: 44; anchors.rightMargin: 44
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 18
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            TrafficLightClose {
                objectName: root.closeButtonObjectName
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.reject()
            }
        }
    }
}
