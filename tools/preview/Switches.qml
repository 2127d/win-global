// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.ksvg as KSvg
import QtQuick.Layouts
import org.kde.desktop as Desktop
import org.kde.plasma.components as Plasma
import org.kde.kirigami as Kirigami

Rectangle {
    Component.onCompleted: {
        KSvg.ImageSet.basePath = "plasma/desktoptheme";
        KSvg.ImageSet.imageSetName = "sequoia-eleven-" + previewMode;
    }

    width: 700; height: 390
    color: Kirigami.Theme.backgroundColor
    Column {
        anchors.fill: parent; anchors.margins: 30; spacing: 24
        Text { text: "Sequoia · Accent switches"; font.pixelSize: 22; color: Kirigami.Theme.textColor }
        Row {
            spacing: 60
            Column {
                spacing: 18
                Text { text: "Applications"; font.pixelSize: 16; color: Kirigami.Theme.textColor }
                Desktop.Switch { text: "On · current accent"; checked: true }
                Desktop.Switch { text: "Off" }
                Desktop.Switch { text: "Unavailable"; checked: true; enabled: false }
            }
            Column {
                spacing: 18
                Text { text: "Plasma widgets"; font.pixelSize: 16; color: Kirigami.Theme.textColor }
                Plasma.Switch { objectName: "plasmaOn"; text: "On · current accent"; checked: true }
                Plasma.Switch { text: "Off" }
                Plasma.Switch { text: "Unavailable"; checked: true; enabled: false }
            }
        }
        Row {
            spacing: 20
            Desktop.MenuItem { width: 220; text: "Menu action · on"; checkable: true; checked: true }
            Desktop.MenuItem { width: 220; text: "Menu action · off"; checkable: true }
        }
    }
}
