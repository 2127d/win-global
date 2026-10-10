// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.iconthemes as IconThemes

QQC2.ScrollView {
    id: settings
    property alias cfg_icon: iconField.text
    property alias cfg_menuWidth: menuWidth.value
    property alias cfg_menuHeight: menuHeight.value
    property alias cfg_pinnedRows: pinnedRows.value
    property alias cfg_gridColumns: columns.value
    property alias cfg_appView: appView.currentIndex
    property alias cfg_showRecommended: recommended.checked
    property alias cfg_recommendationCount: recommendationCount.value
    property alias cfg_recommendationSource: recommendationSource.currentIndex
    property alias cfg_recommendationOrder: recommendationOrder.currentIndex
    property alias cfg_showAllApps: allApps.checked
    property alias cfg_showAccountName: accountName.checked
    property alias cfg_closeOnLaunch: closeOnLaunch.checked
    contentWidth: availableWidth
    QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

    Kirigami.FormLayout {
        width: settings.availableWidth
        Kirigami.Heading { text: qsTr("Make Start feel like home"); level: 2; Kirigami.FormData.isSection: true }
        QQC2.Label {
            text: qsTr("Arrange the same sections you see in the menu. Right-click apps to pin, unpin, or change their order.")
            wrapMode: Text.Wrap; Layout.maximumWidth: 430
            Kirigami.FormData.isSection: true
        }
        RowLayout {
            Kirigami.FormData.label: qsTr("Launcher icon:")
            QQC2.TextField { id: iconField; Layout.fillWidth: true; placeholderText: "start-here-kde"; Accessible.name: qsTr("Launcher icon name") }
            QQC2.Button { icon.name: iconField.text; text: qsTr("Choose…"); onClicked: iconDialog.open() }
            IconThemes.IconDialog { id: iconDialog; onIconNameChanged: iconField.text = iconName }
        }
        RowLayout {
            Kirigami.FormData.label: qsTr("Menu size:")
            QQC2.SpinBox { id: menuWidth; from: 480; to: 960; stepSize: 20; editable: true; Accessible.name: qsTr("Width in pixels") }
            QQC2.Label { text: "×" }
            QQC2.SpinBox { id: menuHeight; from: 480; to: 1000; stepSize: 20; editable: true; Accessible.name: qsTr("Height in pixels") }
        }
        Kirigami.Separator { Kirigami.FormData.isSection: true }
        Kirigami.Heading { text: qsTr("Pinned"); level: 3; Kirigami.FormData.isSection: true }
        QQC2.SpinBox { id: pinnedRows; Kirigami.FormData.label: qsTr("Rows on the main page:"); from: 1; to: 4 }
        QQC2.SpinBox { id: columns; Kirigami.FormData.label: qsTr("Apps per row:"); from: 4; to: 8 }
        Kirigami.Separator { Kirigami.FormData.isSection: true }
        Kirigami.Heading { text: qsTr("Recommended"); level: 3; Kirigami.FormData.isSection: true }
        QQC2.CheckBox { id: recommended; text: qsTr("Show recommendations") }
        QQC2.ComboBox { id: recommendationSource; enabled: recommended.checked; Kirigami.FormData.label: qsTr("Include:"); model: [qsTr("Apps and files"), qsTr("Apps only"), qsTr("Files only")] }
        QQC2.ComboBox { id: recommendationOrder; enabled: recommended.checked; Kirigami.FormData.label: qsTr("Order:"); model: [qsTr("Recently used"), qsTr("Frequently used")] }
        QQC2.SpinBox { id: recommendationCount; enabled: recommended.checked; Kirigami.FormData.label: qsTr("Items on the main page:"); from: 2; to: 12; stepSize: 2 }
        QQC2.Label { text: qsTr("Recommendations use Plasma’s activity history. Hiding this section does not erase that history."); wrapMode: Text.Wrap; Layout.maximumWidth: 430; Kirigami.FormData.isSection: true }
        Kirigami.Separator { Kirigami.FormData.isSection: true }
        Kirigami.Heading { text: qsTr("All apps and footer"); level: 3; Kirigami.FormData.isSection: true }
        QQC2.CheckBox { id: allApps; text: qsTr("Show All apps on the main page") }
        QQC2.ComboBox { id: appView; Kirigami.FormData.label: qsTr("All apps view:"); model: [qsTr("Category"), qsTr("List"), qsTr("Grid")] }
        QQC2.CheckBox { id: accountName; text: qsTr("Show account name beside the avatar") }
        QQC2.CheckBox { id: closeOnLaunch; text: qsTr("Close Start after opening an app or file") }
    }
}
