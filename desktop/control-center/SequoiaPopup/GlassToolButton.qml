// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.ToolButton {
    id: control
    background: GlassActionSurface {
        // Preserve the hit target while keeping icon-only feedback circular.
        width: control.display === PC.AbstractButton.IconOnly ? Math.min(control.width, control.height) : control.width
        height: control.display === PC.AbstractButton.IconOnly ? width : control.height
        x: (control.width - width) / 2
        y: (control.height - height) / 2
        raised: false
        emphasized: control.checked
        pressed: control.down
        hovered: control.hovered && control.enabled
        focused: control.visualFocus
    }
}
