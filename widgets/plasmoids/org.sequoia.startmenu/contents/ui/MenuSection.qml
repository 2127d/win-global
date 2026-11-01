// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.Control {
    id: section
    default property alias sectionContent: body.data
    spacing: 10
    property int motionDuration: 0
    padding: 16
    implicitHeight: body.implicitHeight + topPadding + bottomPadding
    background: MenuSurface { panel: true; elevated: true; motionDuration: section.motionDuration }
    contentItem: Column { id: body; spacing: section.spacing }
}
