// SPDX-FileCopyrightText: 2020-2024 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

InlineChoice {
    id: root

    text: i18n("Speed Units")
    selectedValue: settingsModel.speedUnits
    textRole: "name"
    valueRole: "value"

    onActivated: index => { settingsModel.speedUnits = model.get(index).value; settingsModel.save(); }

    model: ListModel {
        // we can't use i18n with ListElement
        Component.onCompleted: {
            append({
                "name": i18nc("kilometers per hour", "kph"),
                "value": "kph"
            });
            append({
                "name": i18nc("miles per hour", "mph"),
                "value": "mph"
            });
            append({
                "name": i18nc("meters per second", "m/s"),
                "value": "m/s"
            });
        }
    }

}
