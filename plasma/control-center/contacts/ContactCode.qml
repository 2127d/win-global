// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.prison as Prison
import "../details" as Details

PC.Control {
    id: root
    property string qrCodeData
    property int barcodeType: Prison.Barcode.QRCode
    signal typeSelected(int value)
    padding: 20
    background: Details.DetailSurface { radius: 18 }
    contentItem: ColumnLayout {
        spacing: 16
        PC.Label {
            Layout.fillWidth: true
            text: i18n("Share contact")
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }
        PC.Label {
            Layout.fillWidth: true
            text: i18n("Scan this code to add the contact.")
            color: Kirigami.Theme.disabledTextColor
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.centerIn: parent
                width: Math.max(0, Math.min(parent.width, parent.height, 300))
                height: width
                radius: 14
                color: "white"
                Prison.Barcode {
                    id: barcode
                    anchors.fill: parent
                    anchors.margins: 16
                    content: root.qrCodeData
                    barcodeType: root.barcodeType
                    opacity: implicitWidth > 0 && implicitHeight > 0 && implicitWidth <= width && implicitHeight <= height ? 1 : 0
                }
                PC.Label {
                    anchors.fill: parent
                    anchors.margins: 20
                    visible: barcode.opacity === 0
                    text: i18n("This contact cannot fit in the code. Try a different code type or enlarge the menu.")
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#303239"
                }
            }
        }
        PC.ComboBox {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: parent.width
            textRole: "label"
            valueRole: "value"
            Accessible.name: i18n("Code type")
            model: [ {label: i18n("QR Code"), value: Prison.Barcode.QRCode},
                {label: i18n("Data Matrix"), value: Prison.Barcode.DataMatrix},
                {label: i18n("Aztec"), value: Prison.Barcode.Aztec} ]
            currentIndex: Math.max(0, model.findIndex(item => item.value === root.barcodeType))
            onActivated: root.typeSelected(currentValue)
        }
    }
}
