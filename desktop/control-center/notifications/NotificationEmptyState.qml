// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    id: card
    objectName: "notificationEmptyState"
    property string title: qsTr("You're all caught up")
    property string explanation: qsTr("Notifications will appear here when they arrive.")
    property string iconName: "notification-inactive"
    radius: 24
    implicitHeight: content.implicitHeight + 40
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 20; spacing: 12
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56; Layout.preferredHeight: 56; radius: 18
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
            Kirigami.Icon { anchors.centerIn: parent; width: 28; height: 28; source: card.iconName; isMask: true; color: Kirigami.Theme.textColor }
        }
        PC.Label { Layout.fillWidth: true; text: card.title; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap }
        PC.Label { Layout.fillWidth: true; text: card.explanation; visible: text.length > 0; color: Kirigami.Theme.disabledTextColor; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; textFormat: Text.PlainText }
    }
}
