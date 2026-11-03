// SPDX-License-Identifier: MIT
import QtQuick

GlassSurface {
    property bool raised: false
    radius: Math.min(width, height) / 2
    visible: raised || emphasized || hovered || pressed || focused
    shadow.size: raised ? 5 : 0
}
