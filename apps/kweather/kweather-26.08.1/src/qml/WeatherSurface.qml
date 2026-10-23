// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    property bool atmospheric: false
    property bool elevated: false
    readonly property color flatSurface: Qt.tint(Kirigami.Theme.backgroundColor, Qt.alpha(Kirigami.Theme.alternateBackgroundColor, 0.55))
    radius: 24
    color: atmospheric ? (elevated ? "#e0192c42" : "#c518293e") : flatSurface
    border.width: 1
    border.color: atmospheric ? "#20ffffff" : Qt.alpha(Kirigami.Theme.textColor, 0.08)
    Behavior on color { ColorAnimation { duration: Kirigami.Units.longDuration } }
}
