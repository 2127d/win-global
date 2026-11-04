// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import "../details" as Details

FocusScope {
    id: view
    objectName: "connectView"
    required property var devicesModel
    required property Component deviceDelegate
    required property int pairedCount
    required property int discoveredCount
    signal settingsRequested()
    signal storeRequested(string url)
    readonly property real naturalHeight: content.implicitHeight + footer.implicitHeight + 20
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PC.ScrollView {
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: footer.top; bottomMargin: 12 }
        leftPadding: 0; rightPadding: 0
        PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
        contentItem: Flickable {
            clip: true; layer.enabled: true
            contentWidth: width; contentHeight: content.implicitHeight + 8
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: content; width: parent.width; spacing: 12
                Repeater { id: devices; model: view.devicesModel; delegate: view.deviceDelegate }
                Details.DetailSurface {
                    objectName: "connectEmptyCard"
                    Layout.fillWidth: true; radius: 22
                    implicitHeight: emptyBody.implicitHeight + 32
                    visible: devices.count === 0
                    ColumnLayout {
                        id: emptyBody; anchors.fill: parent; anchors.margins: 16; spacing: 12
                        Kirigami.Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 40; Layout.preferredHeight: 40; source: "kdeconnect" }
                        PC.Label {
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; font.weight: Font.DemiBold
                            text: view.pairedCount > 0 ? qsTr("Devices are offline") : qsTr("Connect your devices")
                        }
                        PC.Label {
                            Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap; color: Kirigami.Theme.disabledTextColor
                            text: view.pairedCount > 0 ? qsTr("Open KDE Connect on your paired device and connect it to the same network.") : qsTr("Share files, messages and notifications between your computer and your devices.")
                        }
                        ConnectActionButton {
                            objectName: "connectPairDevice"
                            Layout.alignment: Qt.AlignHCenter
                            visible: view.pairedCount === 0
                            text: qsTr("Pair a device…"); icon.name: "list-add-symbolic"
                            onClicked: view.settingsRequested()
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Qt.alpha(Kirigami.Theme.textColor, 0.1); visible: view.discoveredCount === 0 }
                        PC.Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: qsTr("Get KDE Connect for Android"); horizontalAlignment: Text.AlignHCenter; font: Kirigami.Theme.smallFont; visible: view.discoveredCount === 0 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8; visible: view.discoveredCount === 0
                            ConnectActionButton { Layout.fillWidth: true; Layout.preferredWidth: 1; text: qsTr("Google Play"); icon.name: "download"; onClicked: view.storeRequested("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp") }
                            ConnectActionButton { Layout.fillWidth: true; Layout.preferredWidth: 1; text: qsTr("F-Droid"); icon.name: "download"; onClicked: view.storeRequested("https://f-droid.org/en/packages/org.kde.kdeconnect_tp/") }
                        }
                    }
                }
            }
        }
    }
    Details.DetailSurface {
        id: footer; objectName: "connectFooter"
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, settings.implicitWidth + 16)
        implicitHeight: settings.implicitHeight + 8
        radius: height / 2; elevated: true
        ConnectActionButton {
            id: settings; objectName: "connectSettings"
            anchors.centerIn: parent; flat: true
            text: qsTr("Device settings"); icon.name: "configure-symbolic"
            onClicked: view.settingsRequested()
        }
    }
}
