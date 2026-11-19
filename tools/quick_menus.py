# SPDX-License-Identifier: MIT
"""Shared native-menu presentation for KDE's Qt Quick desktop controls."""


def menu(text, replace):
    text = replace(text, '    margins: 0', '    margins: 8\n    overlap: -6\n    Kirigami.Theme.colorSet: Kirigami.Theme.Window\n    Kirigami.Theme.inherit: false')
    text = replace(text, '    horizontalPadding: style.pixelMetric("menuhmargin")', '    horizontalPadding: 8')
    text = replace(text, '    verticalPadding: style.pixelMetric("menuvmargin")', '    verticalPadding: 8')
    text = replace(text, '        radius: Kirigami.Units.cornerRadius', '        radius: 24')
    text = replace(text, '        border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)',
                   '        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.13)')
    text = replace(text, '        shadow.yOffset: 2', '        shadow.yOffset: 4')
    return replace(text, '        shadow.size: 8', '        shadow.size: 12')


def item(text, replace):
    from menu_toggles import desktop_menu
    text = desktop_menu(text, replace)
    text = replace(text, '    indicator: Loader {', '    indicator: Loader {\n        width: 42\n        height: 24')
    text = replace(text, '            Private.CheckIndicator { drawIcon: false; control: controlRoot }', '''            Item {
                Rectangle {
                    objectName: "sequoiaMenuRadio"
                    anchors.centerIn: parent
                    width: 16; height: 16; radius: 8
                    color: controlRoot.checked ? controlRoot.Kirigami.Theme.highlightColor : Qt.alpha(controlRoot.Kirigami.Theme.textColor, 0.04)
                    border.width: controlRoot.checked ? 0 : 1
                    border.color: Qt.alpha(controlRoot.Kirigami.Theme.textColor, 0.1)
                    opacity: controlRoot.enabled ? 1 : 0.45
                    Rectangle {
                        anchors.centerIn: parent
                        width: 6; height: 6; radius: 3
                        visible: controlRoot.checked
                        color: controlRoot.Kirigami.Theme.backgroundColor
                    }
                }
            }''')
    text = replace(text, '    padding: Kirigami.Units.smallSpacing', '    padding: 5')
    text = replace(text, '    verticalPadding: Kirigami.Settings.hasTransientTouchInput ? 8 : 4 // Hardcoded to the Breeze theme value',
                   '    verticalPadding: Kirigami.Settings.hasTransientTouchInput ? 8 : 5')
    text = replace(text, '    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,',
                   '    implicitHeight: Math.max(34, implicitBackgroundHeight + topInset + bottomInset,')
    text = replace(text, '            color: controlRoot.icon.color',
                   '            color: Qt.colorEqual(controlRoot.icon.color, "transparent") ? Kirigami.Theme.textColor : controlRoot.icon.color')
    text = replace(text, '            selected: controlRoot.pressed', '            selected: false')
    text = replace(text, '            color: controlRoot.pressed ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor',
                   '            color: Kirigami.Theme.textColor')
    text = replace(text, '            color: label.color', '            color: Qt.alpha(label.color, 0.4)')
    text = replace(text, '            Private.SwitchIndicator { control: controlRoot }',
                   '            Private.SwitchIndicator { control: controlRoot; implicitWidth: 42; implicitHeight: 24 }')
    text = replace(text, '        opacity: (controlRoot.highlighted || controlRoot.down) ? 1 : 0',
                   '        opacity: controlRoot.enabled && (controlRoot.highlighted || controlRoot.down) ? 1 : 0')
    text = replace(text, '        color: controlRoot.pressed ? Kirigami.Theme.focusColor : Qt.alpha(Kirigami.Theme.focusColor, 0.3);',
                   '        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.14)')
    text = replace(text, '        border.width: 1', '        border.width: 0')
    return replace(text, '        radius: Kirigami.Units.cornerRadius', '        radius: 13')


def separator(text, replace):
    text = replace(text, '        implicitWidth: Kirigami.Units.gridUnit * 8', '        implicitWidth: Kirigami.Units.gridUnit * 8\n        color: Qt.alpha(Kirigami.Theme.textColor, 0.1)')
    return replace(text, '    verticalPadding: Math.round(Kirigami.Units.smallSpacing / 2)',
                   '    verticalPadding: 5\n    horizontalPadding: 10')
