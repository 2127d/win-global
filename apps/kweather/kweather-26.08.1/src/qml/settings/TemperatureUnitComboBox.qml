// SPDX-FileCopyrightText: 2020-2024 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

InlineChoice {
    id: root

    text: i18n("Temperature Units")
    selectedValue: settingsModel.temperatureUnits
    textRole: "name"
    valueRole: "value"

    onActivated: index => { settingsModel.temperatureUnits = model.get(index).value; settingsModel.save(); }

    model: ListModel {
        // we can't use i18n with ListElement
        Component.onCompleted: {
            append({
                "name": i18n("Use System Default"),
                "value": "Use System Default"
            });
            append({
                "name": i18n("Celsius"),
                "value": "Celsius"
            });
            append({
                "name": i18n("Fahrenheit"),
                "value": "Fahrenheit"
            });
        }
    }

}
