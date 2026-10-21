// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: widget
    objectName: "sequoiaGridTestWidget"
    readonly property bool panel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    preferredRepresentation: fullRepresentation
    Layout.minimumWidth: panel ? Kirigami.Units.iconSizes.smallMedium : 96
    Layout.minimumHeight: panel ? Kirigami.Units.iconSizes.smallMedium : 96
    Layout.preferredWidth: panel ? 96 : 192
    Layout.preferredHeight: panel ? 32 : 192
    fullRepresentation: GridTestFace { panel: widget.panel }
    toolTipMainText: i18n("Sequoia Grid Test")
    toolTipSubText: panel ? i18n("Panel layout") : i18n("Resize in Edit Mode to try different layouts.")
}
