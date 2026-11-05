// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import QtQuick.Templates as T

T.ScrollBar {
    id: bar
    readonly property bool sequoiaStyledScrollBar: true
    policy: T.ScrollBar.AsNeeded
    visible: size > 0 && size < 1
    hoverEnabled: interactive
    minimumSize: Math.min(1, 24 / Math.max(1, height))
    implicitWidth: 8
    padding: 2
    background: null
    contentItem: Rectangle {
        implicitWidth: 4
        radius: width / 2
        color: Kirigami.Theme.textColor
        opacity: bar.pressed ? 0.64 : bar.active || bar.hovered ? 0.38 : 0
        Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
    }
}
