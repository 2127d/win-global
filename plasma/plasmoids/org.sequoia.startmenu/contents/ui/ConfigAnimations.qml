// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

QQC2.ScrollView {
    id: settings
    property alias cfg_animationsEnabled: animations.checked
    property alias cfg_animationDuration: duration.value
    property alias cfg_shrinkOnLaunch: shrink.checked
    contentWidth: availableWidth
    Kirigami.FormLayout {
        width: settings.availableWidth
        Kirigami.Heading { text: qsTr("A little motion, your way"); level: 2; Kirigami.FormData.isSection: true }
        QQC2.CheckBox { id: animations; text: qsTr("Animate page navigation and app launches") }
        QQC2.SpinBox { id: duration; enabled: animations.checked; from: 100; to: 600; stepSize: 20; editable: true; Kirigami.FormData.label: qsTr("Duration (ms):") }
        QQC2.CheckBox { id: shrink; enabled: animations.checked; text: qsTr("Shrink app tiles when opening them") }
        QQC2.Label {
            text: qsTr("Show all, category buttons, and Back slide pages to the left. Disable animations for instant page transitions. Opening and closing use Plasma's desktop effects for the whole menu window.")
            wrapMode: Text.Wrap; Layout.maximumWidth: 430; Kirigami.FormData.isSection: true
        }
        Rectangle {
            Kirigami.FormData.isSection: true
            Layout.preferredWidth: 320; Layout.preferredHeight: 180
            color: Kirigami.Theme.alternateBackgroundColor; radius: 12; clip: true
            Rectangle {
                id: sample
                width: 210; height: 130; radius: 9; x: 55; y: 24
                color: Kirigami.Theme.backgroundColor
                border.color: Kirigami.Theme.disabledTextColor
                Column {
                    anchors.fill: parent; anchors.margins: 16; spacing: 12
                    Rectangle { width: parent.width; height: 12; radius: 6; color: Kirigami.Theme.alternateBackgroundColor }
                    QQC2.Label { text: qsTr("Pinned"); font.bold: true }
                    Row { spacing: 17; Repeater { model: 5; Rectangle { width: 22; height: 22; radius: 5; color: Kirigami.Theme.highlightColor } } }
                }
            }
            SequentialAnimation {
                id: preview
                NumberAnimation { target: sample; property: "x"; from: 320; to: 55; duration: animations.checked ? duration.value : 0; easing.type: Easing.OutCubic }
                PauseAnimation { duration: 350 }
                NumberAnimation { target: sample; property: "scale"; from: 1; to: shrink.checked && animations.checked ? 0.88 : 1; duration: animations.checked ? duration.value / 2 : 0 }
                NumberAnimation { target: sample; property: "scale"; to: 1; duration: animations.checked ? duration.value / 2 : 0 }
                PauseAnimation { duration: 250 }
                NumberAnimation { target: sample; property: "x"; to: -220; duration: animations.checked ? duration.value : 0; easing.type: Easing.InCubic }
                PropertyAction { target: sample; property: "x"; value: 55 }
            }
        }
        QQC2.Button { text: qsTr("Preview animation"); onClicked: preview.restart() }
    }
}
