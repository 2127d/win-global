// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.components as PC

PC.TabBar {
    id: control

    // DetailTabButton paints its own pill-shaped hover and checked states.
    // The native sliding SVG highlight otherwise shows behind those pills.
    Binding {
        target: control.contentItem
        property: "highlight"
        value: null
    }
}
