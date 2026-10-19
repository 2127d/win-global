// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import "../details" as Details

Details.DetailSurface {
    radius: 18
    // Slightly raised against the popup canvas in both desktop appearances.
    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
}
