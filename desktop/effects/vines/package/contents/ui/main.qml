// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import org.kde.kwin as KWin
import "native9" as Vines

// The script owns the window, animation driver, and environment observer.
// Disabling it in KWin Scripts destroys this entire object tree.
Window {
    id: canopy
    property bool jungleEnabled: true
    readonly property var output: KWin.Workspace.screenOrder.length ? KWin.Workspace.screenOrder[0] : KWin.Workspace.screens[0]
    readonly property rect workArea: output ? KWin.Workspace.clientArea(KWin.Workspace.MaximizeArea, output, KWin.Workspace.currentDesktop) : Qt.rect(0,0,1280,800)
    Vines.Environment {
        id: environment
        onSettingsChanged: frameClock.reset()
    }
    KWin.ShortcutHandler {
        name: "SequoiaToggleVines"
        text: "Toggle Jungle Vines"
        sequence: "Meta+Ctrl+M"
        onActivated: canopy.jungleEnabled = !canopy.jungleEnabled
    }
    objectName: "sequoiaJungleVines"
    title: "Sequoia Jungle Vines"
    property bool kwin_shadow_enabled: false
    property bool ready: false
    Component.onCompleted: { environment.prepareWindow(canopy); ready = true; }
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.BypassWindowManagerHint
           | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
    color: "transparent"
    width: canopy.output ? canopy.output.geometry.width : 1280
    height: Math.min(480, Math.round((canopy.output ? canopy.output.geometry.height : 900) * 0.58))
    x: canopy.output ? canopy.output.geometry.x : 0
    y: Math.max(canopy.output ? canopy.output.geometry.y : 0, canopy.workArea.y)
    visible: ready && canopy.jungleEnabled && environment.canShow && canopy.output !== undefined
    Vines.Jungle {
        id: jungle
        anchors.fill: parent
        running: canopy.visible
        swingSpeed: environment.swingSpeed
        pauseSeconds: environment.pauseSeconds
        FrameAnimation {
            id: frameClock
            running: jungle.running
            onRunningChanged: { if (running) reset(); }
            onTriggered: jungle.advance(elapsedTime)
        }
    }
}
