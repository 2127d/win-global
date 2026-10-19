// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.plasmoid

ContactCode {
    property string title: i18n("Share contact")
    barcodeType: Plasmoid.configuration.barcodeType
    onTypeSelected: value => Plasmoid.configuration.barcodeType = value
}
