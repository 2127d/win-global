// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.ToolButton {
    id: control
    background: GlassActionSurface {
        raised: false
        emphasized: control.checked
        pressed: control.down
        hovered: control.hovered && control.enabled
        focused: control.visualFocus
    }
}
