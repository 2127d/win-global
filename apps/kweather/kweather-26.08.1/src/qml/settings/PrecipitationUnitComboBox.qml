// SPDX-FileCopyrightText: 2020-2024 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

InlineChoice {
    id: root

    text: i18n("Precipitation Units")
    selectedValue: settingsModel.precipitationUnits
    textRole: "name"
    valueRole: "value"

    onActivated: index => { settingsModel.precipitationUnits = model.get(index).value; settingsModel.save(); }

    model: ListModel {
        // we can't use i18n with ListElement
        Component.onCompleted: {
            append({
                "name": i18nc("Millimeters", "mm"),
                "value": "mm"
            });
            append({
                "name": i18nc("Inches", "in"),
                "value": "in"
            });
        }
    }

}
