// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.Button {
    id: control
    background: GlassActionSurface {
        raised: true
        emphasized: control.checked
        pressed: control.down
        hovered: control.hovered && control.enabled
        focused: control.visualFocus
    }
}
