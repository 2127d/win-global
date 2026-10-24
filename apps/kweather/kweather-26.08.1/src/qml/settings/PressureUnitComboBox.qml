// SPDX-FileCopyrightText: 2020-2024 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

InlineChoice {
    id: root

    text: i18n("Pressure Units")
    selectedValue: settingsModel.pressureUnits
    textRole: "name"
    valueRole: "value"

    onActivated: index => { settingsModel.pressureUnits = model.get(index).value; settingsModel.save(); }

    model: ListModel {
        // we can't use i18n with ListElement
        Component.onCompleted: {
            append({
                "name": i18nc("Hectopascal Pressure", "hPa"),
                "value": "hPa"
            });
            append({
                "name": i18nc("Millimetre of mercury", "mmHg"),
                "value": "mmHg"
            });
        }
    }

}
