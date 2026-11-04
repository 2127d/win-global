// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../../details" as Details

Details.DetailSurface {
    id: card
    property bool searching: false
    property bool starred: false
    objectName: "clipboardEmptyState"
    radius: 24
    implicitHeight: content.implicitHeight + 40
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 20; spacing: 12
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 56; Layout.preferredHeight: 56; radius: 18
            color: Qt.alpha(Kirigami.Theme.highlightColor,0.12)
            Kirigami.Icon { anchors.centerIn: parent; width: 28; height: 28; source: card.searching ? "edit-find-symbolic" : card.starred ? "non-starred-symbolic" : "edit-paste-symbolic"; isMask: true; color: Kirigami.Theme.textColor }
        }
        PC.Label { Layout.fillWidth: true; text: card.searching ? qsTr("No matching items") : card.starred ? qsTr("Keep useful copies close") : qsTr("Ready when you copy"); font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap }
        PC.Label {
            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
            color: Kirigami.Theme.disabledTextColor
            text: card.searching ? qsTr("Try a different search or look in all history.") : card.starred ? qsTr("Star an item in History to find it here later.") : qsTr("Copy text, images, or files. Choose an item here to copy it again.")
        }
    }
}
