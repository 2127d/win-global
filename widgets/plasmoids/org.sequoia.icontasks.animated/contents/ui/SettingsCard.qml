// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.Frame {
    id: card
    property string title
    default property alias contents: body.data
    Layout.fillWidth: true
    padding: Kirigami.Units.largeSpacing * 1.5
    background: Rectangle {
        radius: 18
        color: Qt.tint(Kirigami.Theme.backgroundColor, Kirigami.Theme.backgroundColor.r < 0.5 ? "#0effffff" : "#80ffffff")
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
    }
    contentItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            Layout.fillWidth: true
            text: card.title
            level: 3
            font.weight: Font.DemiBold
            wrapMode: Text.Wrap
        }
        ColumnLayout { id: body; Layout.fillWidth: true; spacing: Kirigami.Units.smallSpacing }
    }
}
