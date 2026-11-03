// SPDX-License-Identifier: MIT
import QtQuick
import "SequoiaPopup" as Popup

Popup.GlassSurface {
    hovered: hoverHandler.hovered
    HoverHandler { id: hoverHandler }
}
