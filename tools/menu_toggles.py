# SPDX-License-Identifier: MIT
"""Apply menu indicators while preserving KDE's action and exclusive-group logic."""


def desktop_menu(text, replace):
    old = '''    indicator: Private.CheckIndicator {
        x: controlRoot.mirrored ? controlRoot.width - width - controlRoot.rightPadding : controlRoot.leftPadding
        y: controlRoot.topPadding + (controlRoot.availableHeight - height) / 2

        drawIcon: false // We're drawing it ourselves in this control

        visible: controlRoot.checkable
        on: controlRoot.checked
        control: controlRoot
    }'''
    new = '''    indicator: Loader {
        x: controlRoot.mirrored ? controlRoot.width - width - controlRoot.rightPadding : controlRoot.leftPadding
        y: controlRoot.topPadding + (controlRoot.availableHeight - height) / 2
        visible: controlRoot.checkable
        readonly property bool exclusive: controlRoot.autoExclusive
            || (controlRoot.action !== null && controlRoot.action.T.ActionGroup.group !== null && controlRoot.action.T.ActionGroup.group.exclusive)
            || (controlRoot.T.ButtonGroup.group !== null && controlRoot.T.ButtonGroup.group.exclusive)
        sourceComponent: exclusive ? radioIndicator : toggleIndicator
        Component {
            id: radioIndicator
            Private.CheckIndicator { drawIcon: false; control: controlRoot }
        }
        Component {
            id: toggleIndicator
            Private.SwitchIndicator { control: controlRoot }
        }
    }'''
    return replace(text, old, new)
