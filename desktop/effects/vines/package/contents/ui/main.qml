// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import org.kde.kwin as KWin
import "native2" as Vines

// An internal output-only window avoids SceneEffect's fullscreen input grab.
// Its lifetime still belongs to the KWin effect: unloading destroys everything.
KWin.SceneEffect {
    id: effect
    property bool jungleEnabled: true
    readonly property var output: KWin.Workspace.screenOrder.length ? KWin.Workspace.screenOrder[0] : KWin.Workspace.screens[0]
    readonly property rect workArea: output ? KWin.Workspace.clientArea(KWin.Workspace.MaximizeArea, output, KWin.Workspace.currentDesktop) : Qt.rect(0,0,1280,800)
    Vines.Environment { id: environment }
    KWin.ShortcutHandler {
        name: "SequoiaToggleVines"
        text: "Toggle Jungle Vines"
        sequence: "Meta+Ctrl+M"
        onActivated: effect.jungleEnabled = !effect.jungleEnabled
    }
    Window {
        id: canopy
        objectName: "sequoiaJungleVines"
        title: "Sequoia Jungle Vines"
        property bool kwin_shadow_enabled: false
        property bool ready: false
        Component.onCompleted: { environment.prepareWindow(canopy); ready = true; }
        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.BypassWindowManagerHint
               | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
        color: "transparent"
        width: effect.output ? effect.output.geometry.width : 1280
        height: Math.min(396, Math.round((effect.output ? effect.output.geometry.height : 900) * 0.48))
        x: effect.output ? effect.output.geometry.x : 0
        y: Math.max(effect.output ? effect.output.geometry.y : 0, effect.workArea.y)
        visible: ready && effect.jungleEnabled && environment.canShow && effect.output !== undefined
        Vines.Jungle {
            anchors.fill: parent
            running: canopy.visible
        }
    }
}
