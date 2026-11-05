// SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.networkmanagement as PlasmaNM
import org.kde.kquickcontrolsaddons as KQuickControlsAddons

Item {
    id: root
    property PlasmaNM.ConnectionDetailsModel detailsModel
    height: fields.implicitHeight
    KQuickControlsAddons.Clipboard { id: clipboard }
    NetworkMenu {
        id: copyMenu
        property string value
        NetworkMenuItem {
            text: i18n("Copy")
            icon.name: "edit-copy"
            onTriggered: clipboard.content = copyMenu.value
        }
    }
    ColumnLayout {
        id: fields
        x: 8
        width: Math.max(0, parent.width - 16)
        spacing: 4
        Repeater {
            model: root.detailsModel
            delegate: ColumnLayout {
                id: field
                required property bool isSection
                required property string sectionTitle
                required property string detailLabel
                required property string detailValue
                Layout.fillWidth: true
                visible: isSection || detailLabel.length > 0 || detailValue.length > 0
                PC.Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    visible: field.isSection
                    text: field.sectionTitle
                    font.weight: Font.DemiBold
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    visible: !field.isSection
                    spacing: 12
                    PC.Label {
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 2
                        Layout.alignment: Qt.AlignTop
                        text: field.detailLabel
                        textFormat: Text.PlainText; wrapMode: Text.Wrap
                        color: Kirigami.Theme.disabledTextColor
                    }
                    PC.Label {
                        id: valueLabel
                        Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 3
                        Layout.alignment: Qt.AlignTop
                        text: field.detailValue
                        textFormat: Text.PlainText; wrapMode: Text.WrapAnywhere
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            enabled: field.detailValue.length > 0
                            onClicked: event => {
                                copyMenu.value = field.detailValue;
                                copyMenu.popup(valueLabel, event.x, event.y);
                            }
                        }
                    }
                }
            }
        }
    }
}
