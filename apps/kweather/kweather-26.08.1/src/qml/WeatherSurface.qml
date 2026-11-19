// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.kirigami as Kirigami

WeatherGlassSurface {
    id: surface
    property bool elevated: false
    panelMaterial: true
    // A reading tint lets scenery through while supporting the brighter
    // forecast captions. Rim intensity remains independent of this fill.
    fillOpacity: 1 - Math.max(0, Math.min(100, atmospheric
        ? settingsModel.cardTransparency : settingsModel.flatCardTransparency)) / 100
    tint: theme.windowColor
    radius: 24
    // Quiet edges keep the forecast content ahead of the glass treatment.
    refractionStrength: 12
    bevelWidth: 8
    shadow: false
}
