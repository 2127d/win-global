// SPDX-License-Identifier: LGPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

QQC2.AbstractButton {
    id: card
    required property Discover.AbstractResource application
    property bool showcase: false
    property real imageHeight: 164
    readonly property color surfaceColor: Qt.rgba(Kirigami.Theme.backgroundColor.r,
        Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 1)
    signal revealRequested(var item)

    objectName: showcase ? "sequoiaShowcaseApp" : "sequoiaCompactApp"
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    padding: 14
    implicitHeight: (showcase ? imageHeight + 10 : 0) + Math.max(44, details.implicitHeight) + topPadding + bottomPadding
    implicitWidth: 240
    Accessible.name: application.name
    Accessible.description: application.comment
    onClicked: Navigation.openApplication(application)
    onActiveFocusChanged: if (activeFocus) revealRequested(card)

    background: Kirigami.ShadowedRectangle {
        radius: card.showcase ? 22 : 16
        color: Qt.tint(card.surfaceColor,
                       Qt.alpha(card.Kirigami.Theme.textColor, card.down ? 0.11 : card.hovered ? 0.075 : 0.035))
        border.width: card.activeFocus ? 2 : 1
        border.color: card.activeFocus ? card.Kirigami.Theme.highlightColor
            : Qt.alpha(card.Kirigami.Theme.textColor, card.hovered ? 0.22 : 0.13)
        shadow.size: 6
        shadow.xOffset: 0
        shadow.yOffset: card.down ? 1 : 2
        shadow.color: Qt.rgba(0, 0, 0, card.Kirigami.Theme.textColor.hslLightness > 0.5 ? 0.28 : 0.12)
    }
    contentItem: ColumnLayout {
        spacing: card.showcase ? 10 : 0
        Loader {
            Layout.fillWidth: true
            Layout.preferredHeight: card.imageHeight
            active: card.showcase
            visible: active
            sourceComponent: Kirigami.ShadowedRectangle {
                id: preview
                color: Qt.tint(card.surfaceColor, Qt.alpha(card.Kirigami.Theme.highlightColor, 0.08))
                radius: 12
                border.width: 1
                border.color: Qt.alpha(card.Kirigami.Theme.textColor, 0.10)
                shadow.size: 8
                shadow.xOffset: 0
                shadow.yOffset: 2
                shadow.color: Qt.rgba(0, 0, 0, card.Kirigami.Theme.textColor.hslLightness > 0.5 ? 0.30 : 0.16)
                Discover.ScreenshotsModel {
                    id: screenshots
                    application: card.application
                }
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: 64
                    height: 64
                    source: card.application.icon
                    visible: (images.itemAt(0)?.status ?? Image.Null) !== Image.Ready
                }
                Repeater {
                    id: images
                    model: screenshots
                    delegate: Image {
                        required property int index
                        required property url small_image_url
                        required property url large_image_url
                        anchors.fill: parent
                        anchors.margins: 4
                        visible: index === 0 && status === Image.Ready
                        source: index === 0 ? (large_image_url.toString().length ? large_image_url : small_image_url) : ""
                        sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        retainWhileLoading: true
                    }
                }
            }
        }
        RowLayout {
            id: details
            Layout.fillWidth: true
            spacing: 12
            Kirigami.Icon {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                source: card.application.icon
                animated: false
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                QQC2.Label {
                    Layout.fillWidth: true
                    text: card.application.name
                    textFormat: Text.PlainText
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: card.application.comment
                    textFormat: Text.PlainText
                    font: Kirigami.Theme.smallFont
                    opacity: 0.75
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }
        }
    }
    QQC2.ToolTip.text: application.name + "\n" + application.comment
    QQC2.ToolTip.visible: hovered
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
}
