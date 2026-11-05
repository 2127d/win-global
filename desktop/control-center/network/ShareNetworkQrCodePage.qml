// SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <kde@broulik.de>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.prison as Prison
import "../details" as Details

PC.ScrollView {
    id: page
    property alias content: barcode.content
    property string ssid
    property string password
    readonly property real naturalHeight: card.implicitHeight
    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
    background: null
    PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff
    PC.ScrollBar.vertical: NetworkScrollBar {
        parent: page
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom; margins: 4 }
    }
    contentItem: Flickable {
        clip: true
        contentWidth: width
        contentHeight: card.height
        boundsBehavior: Flickable.StopAtBounds
        Details.DetailSurface {
            id: card
            width: parent.width
            implicitHeight: body.implicitHeight + 32
            radius: 24
            ColumnLayout {
                id: body
                x: 16; y: 16; width: Math.max(0, parent.width - 32)
                spacing: 10
                PC.Label {
                    Layout.fillWidth: true
                    text: page.ssid
                    textFormat: Text.PlainText; wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    font.weight: Font.DemiBold
                }
                PC.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Kirigami.Theme.disabledTextColor
                    text: qsTr("Scan to connect to this network")
                }
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(body.width, 224)
                    Layout.preferredHeight: Layout.preferredWidth
                    radius: 12; color: "white"
                    Prison.Barcode {
                        id: barcode
                        anchors.fill: parent; anchors.margins: 16
                        barcodeType: Prison.Barcode.QRCode
                        Drag.dragType: Drag.Automatic
                        Drag.supportedActions: Qt.CopyAction
                        HoverHandler { enabled: barcode.implicitWidth > 0; cursorShape: Qt.OpenHandCursor }
                        DragHandler {
                            id: drag
                            enabled: barcode.implicitWidth > 0
                            cursorShape: Qt.ClosedHandCursor
                            target: null
                            onActiveChanged: {
                                if (active) barcode.grabToImage(result => {
                                    barcode.Drag.mimeData = { "image/png": result.image };
                                    barcode.Drag.active = drag.active;
                                });
                                else barcode.Drag.active = false;
                            }
                        }
                    }
                }
                PC.Label {
                    Layout.fillWidth: true
                    visible: page.password.length > 0
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.disabledTextColor
                    text: i18n("Wi-Fi password:")
                }
                Kirigami.SelectableLabel {
                    Layout.fillWidth: true; Layout.minimumWidth: 0
                    visible: page.password.length > 0
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: TextEdit.WrapAnywhere; textFormat: TextEdit.PlainText
                    font.family: "monospace"
                    text: page.password
                }
            }
        }
    }
}
