// SPDX-License-Identifier: MIT
import QtQuick
import "../details" as Details

Details.DetailSurface {
    objectName: "audioMenuSurface"
    radius: 24
    elevated: true
    // Menus overlay live controls; keep the glass readable without window blur.
    color: Qt.alpha(stateColor, dark ? 0.97 : 0.98)
    shadow.size: 12
    shadow.yOffset: 3
}
