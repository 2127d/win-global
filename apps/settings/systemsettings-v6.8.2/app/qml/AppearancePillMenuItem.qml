// SPDX-License-Identifier: GPL-2.0-or-later
import org.sequoia.components 1.0 as S

AppearanceMenuItem {
    id: menuItem
    background: S.PillHighlight {
        theme: menuItem.theme
        highlighted: menuItem.highlighted
        hovered: menuItem.hovered
        pressed: menuItem.down
        focused: menuItem.visualFocus
        enabled: menuItem.enabled
    }
}
