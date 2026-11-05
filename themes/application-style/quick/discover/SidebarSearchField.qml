// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
import QtQuick
import org.kde.kirigami as Kirigami

SearchField {
    id: field
    implicitHeight: Math.max(40, Math.ceil(contentHeight) + 20)
    topPadding: 10
    bottomPadding: 10
    leftPadding: LayoutMirroring.enabled ? _rightActionsRow.width + 12 : 38
    rightPadding: LayoutMirroring.enabled ? 38 : _rightActionsRow.width + 12
    sequoiaIconInset: 14
    sequoiaIconSize: 16
    _rightActionsRow.anchors.margins: 6
    clip: true

    background: Rectangle {
        id: searchSurface
        objectName: "sequoiaSidebarSearchBackground"
        radius: height / 2
        color: Qt.tint(Qt.rgba(field.Kirigami.Theme.backgroundColor.r,
                               field.Kirigami.Theme.backgroundColor.g,
                               field.Kirigami.Theme.backgroundColor.b, 1),
                       Qt.alpha(field.Kirigami.Theme.textColor, 0.025))
        border.width: field.activeFocus ? 2 : 1
        border.color: field.activeFocus ? Qt.alpha(field.Kirigami.Theme.highlightColor, 0.75)
            : Qt.alpha(field.Kirigami.Theme.textColor, field.hovered ? 0.22 : 0.13)

        // Paint behind the clipped field so the shadow can extend beyond it
        // while the search icon, text, and clear button stay contained.
        Kirigami.ShadowedRectangle {
            objectName: "sequoiaSidebarSearchShadow"
            parent: field.parent
            x: field.x
            y: field.y
            width: field.width
            height: field.height
            z: field.z - 1
            visible: field.visible
            radius: height / 2
            color: searchSurface.color
            shadow.size: 6
            shadow.xOffset: 0
            shadow.yOffset: 2
            shadow.color: Qt.rgba(0, 0, 0, field.Kirigami.Theme.textColor.hslLightness > 0.5 ? 0.26 : 0.12)
        }
    }
}
