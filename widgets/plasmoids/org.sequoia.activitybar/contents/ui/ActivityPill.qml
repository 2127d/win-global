// SPDX-License-Identifier: GPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami

Item {
    id: pill
    objectName: "sequoiaActivityPill"
    required property var model
    property Item wallpaper: null
    signal activated(string activityId)
    // The complete footprint includes the standard six-pixel inset on each side.
    readonly property int cell: 96
    implicitWidth: Math.max(2 * cell, Math.ceil((tabs.implicitWidth + 36) / cell) * cell)
    implicitHeight: Math.max(cell, Math.ceil((tabs.implicitHeight + 36) / cell) * cell)

    PureGlassSurface {
        objectName: "activityPillSurface"
        anchors.fill: parent
        anchors.margins: 6
        radius: height / 2
        wallpaper: pill.wallpaper
    }
    Row {
        id: tabs
        objectName: "activityPillTabs"
        anchors.centerIn: parent
        layoutDirection: pill.LayoutMirroring.enabled ? Qt.RightToLeft : Qt.LeftToRight
        Repeater {
            model: pill.model
            delegate: Controls.AbstractButton {
                id: tab
                required property string name
                required property string id
                required property bool current
                objectName: "activityPillTab"
                text: name
                checked: current
                focusPolicy: Qt.StrongFocus
                hoverEnabled: true
                leftPadding: 12
                rightPadding: 12
                topPadding: 10
                bottomPadding: 10
                implicitWidth: label.implicitWidth + leftPadding + rightPadding
                implicitHeight: label.implicitHeight + topPadding + bottomPadding
                Accessible.name: name
                Accessible.description: qsTr("Switch to activity %1").arg(name)
                Accessible.role: Accessible.Button
                Accessible.checked: current
                onClicked: pill.activated(id)
                Keys.onReturnPressed: pill.activated(id)
                Keys.onEnterPressed: pill.activated(id)
                contentItem: PC3.Label {
                    id: label
                    text: tab.text
                    textFormat: Text.PlainText
                    color: "white"
                    style: Text.Outline
                    styleColor: Qt.rgba(0, 0, 0, 0.55)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: height / 2
                    color: Qt.alpha(Kirigami.Theme.highlightColor, tab.down ? .20 : tab.hovered ? .10 : 0)
                    border.width: tab.visualFocus ? 1 : 0
                    border.color: Kirigami.Theme.highlightColor
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 3
                        width: label.implicitWidth
                        height: 3
                        radius: 1.5
                        visible: tab.current
                        color: Kirigami.Theme.highlightColor
                    }
                }
            }
        }
    }
}
