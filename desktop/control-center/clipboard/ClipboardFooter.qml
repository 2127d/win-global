// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../../details" as Details

Details.DetailSurface {
    id: footer
    required property var clipboardMenu
    signal configureRequested()
    objectName: "clipboardActionIsland"
    width: Math.min(parent.width, actions.implicitWidth + 16)
    implicitHeight: actions.implicitHeight + 8
    radius: height / 2; elevated: true
    RowLayout {
        id: actions; anchors.centerIn: parent; spacing: 2
        ClipboardActionButton { objectName: "clipboardSettings"; flat: true; text: qsTr("Settings"); icon.name: "configure-symbolic"; onClicked: footer.configureRequested() }
        Rectangle { width: 1; height: 18; color: Qt.alpha(Kirigami.Theme.textColor,0.15) }
        ClipboardActionButton { objectName: "clipboardClearHistory"; flat: true; text: qsTr("Clear history"); icon.name: "edit-clear-history"; enabled: (footer.clipboardMenu?.model.sourceCount ?? 0) > 0; onClicked: footer.clipboardMenu.clearHistory() }
    }
}
