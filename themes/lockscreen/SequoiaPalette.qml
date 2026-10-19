// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

Item {
    // Read the desktop palette independently of the lock screen's overrides.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    property string mode: "auto"
    readonly property color desktopBackground: Kirigami.Theme.backgroundColor
    readonly property bool dark: mode === "dark" || (mode === "auto" && desktopBackground.hslLightness < 0.5)
    readonly property color foreground: dark ? "#f4f7fa" : "#172c38"
    readonly property color muted: dark ? "#becbd2" : "#475e6d"
    readonly property color surface: dark ? "#dc18262d" : "#e5edf4f6"
    readonly property color glass: dark ? "#90121d22" : "#bbecf3f6"
    readonly property color border: dark ? "#607b929c" : "#8094aab5"
    readonly property color field: dark ? "#50202b30" : "#baffffff"
    readonly property color focusColor: dark ? "#a9d5ed" : "#2463d4"
    readonly property color accent: dark ? "#2da1f6" : "#2463d4"
}
