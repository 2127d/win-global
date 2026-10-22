/*
    SPDX-FileCopyrightText: 2013 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    property string cfg_faceStyle: "sequoia"
    property alias cfg_showSecondHand: showSecondHandCheckBox.checked
    property alias cfg_showTimezoneString: showTimezoneCheckBox.checked

    Kirigami.FormLayout {
        ComboBox {
            Kirigami.FormData.label: i18n("Clock face:")
            model: [i18n("Sequoia"), i18n("Original")]
            currentIndex: cfg_faceStyle === "original" ? 1 : 0
            onActivated: index => cfg_faceStyle = index === 1 ? "original" : "sequoia"
        }
        CheckBox {
            id: showSecondHandCheckBox
            text: i18n("Show seconds hand")
            Kirigami.FormData.label: i18n("General:")
        }
        CheckBox {
            id: showTimezoneCheckBox
            text: i18n("Show time zone")
        }
    }
}
