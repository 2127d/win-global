// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Templates as T
import org.kde.kirigami as K

// Native TabBar semantics, with one sliding glass selection for its tabs.
// Pure QML so the same library source can ship in standalone plasmoids.
T.TabBar {
    id: control
    property var theme: null
    readonly property color surfaceColor: theme ? theme.windowColor : K.Theme.backgroundColor
    readonly property color textColor: theme ? theme.textColor : K.Theme.textColor
    readonly property color accentColor: theme ? theme.accentColor : K.Theme.highlightColor
    readonly property int selectionDuration: theme ? theme.selectionDuration : K.Units.longDuration
    padding: 4
    spacing: 2
    implicitWidth: contentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(38, contentHeight + topPadding + bottomPadding)
    background: GlassPillSurface {
        objectName: "glassPillTrack"
        surfaceColor: control.surfaceColor
        foregroundColor: control.textColor
    }
    contentItem: ListView {
        implicitWidth: contentWidth
        implicitHeight: contentHeight
        model: control.contentModel
        currentIndex: control.currentIndex
        orientation: ListView.Horizontal
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
        spacing: control.spacing
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentWidth > width
        highlightMoveDuration: control.selectionDuration
        highlightResizeDuration: control.selectionDuration
        highlight: GlassPillSurface {
            objectName: "glassPillSelection"
            surfaceColor: control.surfaceColor
            foregroundColor: control.textColor
            selected: true
        }
    }
}
