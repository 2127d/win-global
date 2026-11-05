// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC
import QtQuick.Templates as T
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import "../details" as Details

T.ComboBox {
            hoverEnabled: true
            id: picker
            readonly property var selector: picker
            required property var record
            required property var devices
            required property var controller
            property bool input: false
            property string streamId: ""
            implicitWidth: 180; implicitHeight: 36
            objectName: streamId ? "audioRoute:" + streamId : (selector.input ? "audioInputPicker" : "audioOutputPicker")
            Layout.fillWidth: true
            enabled: selector.devices.length > 1
            model: selector.devices
            textRole: "name"
            displayText: selector.record.name
            leftPadding: mirrored ? 36 : 12; rightPadding: mirrored ? 12 : 36; topPadding: 6; bottomPadding: 6
            contentItem: PC.Label { text: picker.displayText; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; color: Kirigami.Theme.textColor }
            indicator: Kirigami.Icon { x: picker.mirrored ? 12 : picker.width - width - 12; y: (picker.height - height) / 2; width: 16; height: 16; source: "arrow-down-symbolic"; isMask: true; color: Kirigami.Theme.textColor }
            delegate: AudioMenuItem {
                required property var modelData
                required property int index
                width: picker.popup.availableWidth
                text: modelData.name
                highlighted: picker.highlightedIndex === index
                checked: picker.currentIndex === index
            }
            popup: T.Popup {
                objectName: "audioDevicePickerMenu"
                popupType: QQC.Popup.Item
                y: picker.height + 4
                width: picker.width
                padding: 8; margins: 12
                implicitHeight: pickerList.contentHeight + topPadding + bottomPadding
                height: Math.min(implicitHeight, Math.max(1,(picker.Window.window?.height || 600) - 2 * margins))
                background: AudioMenuSurface {}
                contentItem: ListView {
                    id: pickerList
                    clip: true; spacing: 2
                    flickableDirection: Flickable.VerticalFlick
                    interactive: contentHeight > height
                    layer.enabled: interactive
                    model: picker.delegateModel
                    currentIndex: picker.highlightedIndex
                    boundsBehavior: Flickable.StopAtBounds
                    QQC.ScrollBar.vertical: Details.DetailScrollBar {}
                }
            }
            currentIndex: selector.devices.findIndex(d => d.id === selector.record.id)
            Accessible.name: streamId ? qsTr("App audio device") : (selector.input ? qsTr("Microphone device") : qsTr("Sound output device"))
            onActivated: index => { const device = selector.devices[index]; if (device) { if (streamId) selector.controller.routeStream(streamId,device.id); else selector.controller.selectDevice(device.id); } }
            background: Details.DetailSurface { radius: height / 2; shadow.size: 0; hovered: picker.hovered; focused: picker.activeFocus }
        }
