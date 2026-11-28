# SPDX-License-Identifier: MIT
"""Tooltip geometry shared by the Hub, Qt Quick apps and Plasma controls."""
from quick_islands import background, replace


def desktop(text):
    text = replace(text, '    padding: 6', '''    padding: 10
    horizontalPadding: 18
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0''')
    text = text.replace('import QtQuick\n', 'import QtQuick\nimport org.sequoia.glass 1.0 as Glass\n', 1)
    text = background(text, '''    background: Rectangle {
        objectName: "sequoiaTooltipCapsule"
        readonly property string shape: {
            Glass.Settings.tooltipRevision;
            return Glass.Settings.tooltipShape(Glass.Settings.applicationSection);
        }
        radius: shape === "pill" ? Math.min(width, height) / 2 : shape === "rounded" ? Math.min(20, Math.min(width, height) / 2) : 0
        antialiasing: true
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.14)
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Math.max(0, parent.radius - 1)
            antialiasing: true
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, Kirigami.Theme.backgroundColor.hslLightness < .5 ? .16 : .55)
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(1, 1, 1, .12) }
                GradientStop { position: .5; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, .025) }
            }
        }
    }''')
    return text


def plasma(text):
    from quick_islands import QML
    desktop_source = desktop((QML / 'org/kde/desktop/ToolTip.qml').read_text())
    surface = desktop_source[desktop_source.index('    background: Rectangle {'):desktop_source.rfind('}')]
    surface = surface.replace('Glass.Settings.applicationSection', '"Desktop"')
    text = text.replace('import QtQuick\n', 'import QtQuick\nimport org.sequoia.glass 1.0 as Glass\n', 1)
    for edge in ('top', 'bottom'):
        text = replace(text, edge+'Padding: backgroundItem.margins.'+edge, edge+'Padding: 10')
    for edge in ('left', 'right'):
        text = replace(text, edge+'Padding: backgroundItem.margins.'+edge, edge+'Padding: 18')
    return background(text, surface)
