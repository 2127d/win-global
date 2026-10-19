// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.ItemDelegate {
    id: control
    padding: 12
    topInset: 3
    bottomInset: 3
    background: DetailSurface {
        objectName: "sequoiaDetailCard"
        emphasized: control.highlighted
        hovered: control.hovered
        focused: control.activeFocus || (control.ListView.isCurrentItem && !!control.ListView.view?.activeFocus)
    }
}
