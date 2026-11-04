// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    objectName: "vaultsView"
    required property var controller
    required property Component vaultDelegate
    readonly property real naturalHeight: content.implicitHeight + footer.implicitHeight + 20
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PC.ScrollView {
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footer.top; bottomMargin: 12 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true; contentWidth: width; contentHeight: content.implicitHeight + 8
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content; width: parent.width; spacing: 12
                Repeater { id: vaults; model: view.controller.vaultsModel; delegate: view.vaultDelegate }
                Details.DetailSurface {
                    objectName: "vaultEmptyCard"
                    Layout.fillWidth: true; radius: 22; implicitHeight: emptyBody.implicitHeight + 32
                    visible: vaults.count === 0
                    ColumnLayout {
                        id: emptyBody; anchors.fill: parent; anchors.margins: 16; spacing: 12
                        Kirigami.Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 40; Layout.preferredHeight: 40; source: "plasmavault" }
                        PC.Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: qsTr("No vaults yet"); wrapMode: Text.Wrap; font.weight: Font.DemiBold }
                        PC.Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: qsTr("Create an encrypted folder to keep your files private."); wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer; objectName: "vaultFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width,create.implicitWidth + 16)
        implicitHeight: create.implicitHeight + 8; radius: height / 2; elevated: true
        VaultActionButton { id: create; objectName: "vaultCreate"; anchors.centerIn: parent; flat: true; text: qsTr("Create vault…"); icon.name: "list-add-symbolic"; onClicked: view.controller.create() }
    }
}
