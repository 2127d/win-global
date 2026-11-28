// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import org.kde.ksvg as KSvg
import org.kde.kirigami as K
import org.sequoia.glass 1.0 as Glass
Item {
    id: root
    anchors.fill: parent
    property alias imagePath: frame.imagePath
    property alias enabledBorders: frame.enabledBorders
    readonly property bool tooltip: imagePath.indexOf("tooltip") !== -1
    readonly property var popup: root.Window.window
    // The native Dialog base constructs this component before mainItem exists.
    readonly property var tooltipItem: tooltip && (popup?.visible ?? false) ? popup.mainItem : null
    readonly property bool running: tooltipItem?.isWin === true || tooltipItem?.sequoiaRunningAppTooltip === true
    readonly property string shape: {
        Glass.Settings.tooltipRevision;
        return Glass.Settings.tooltipShape(running ? "RunningApps" : "Desktop");
    }
    property bool spaced: false
    property int oldMargin: 0
    function present() {
        if (!tooltip || !popup?.visible) return;
        // Remove the legacy rectangular shadow backing from custom surfaces.
        if (popup.borders !== 0) popup.borders = 0;
        if (running) {
            if (!spaced) { oldMargin = surface.popupMargin(); spaced = true; }
            if (surface.popupMargin() !== 12) surface.setPopupMargin(12);
        } else if (spaced) { surface.setPopupMargin(oldMargin); spaced = false; }
    }
    onRunningChanged: Qt.callLater(present)
    Connections {
        target: root.popup
        ignoreUnknownSignals: true
        function onVisibleChanged() { Qt.callLater(root.present); }
        function onBordersChanged() { Qt.callLater(root.present); }
    }
    readonly property var mask: tooltip ? surface.mask : frame.mask
    readonly property var fixedMargins: tooltip ? padding : frame.fixedMargins
    readonly property var inset: tooltip ? zero : frame.inset
    QtObject {
        id: padding
        // These are inputs to Plasma's content layout. Deriving them from
        // the resulting popup height feeds wrapping back into its own size.
        // Keep the pill shape in the paint radius, not in layout margins.
        readonly property real left: root.running ? 16 : 18
        readonly property real right: left
        readonly property real top: root.running ? 12 : 10
        readonly property real bottom: top
        signal marginsChanged()
        onLeftChanged: marginsChanged()
        onTopChanged: marginsChanged()
    }
    QtObject { id: zero; readonly property real left:0; readonly property real right:0; readonly property real top:0; readonly property real bottom:0 }
    KSvg.FrameSvgItem { id: frame; anchors.fill:parent; imagePath:"widgets/background"; visible:!root.tooltip }
    Glass.TooltipSurface {
        id: surface; anchors.fill:parent; visible:root.tooltip
        radius: root.shape === "pill" ? Math.min(width,height)/2 : root.shape === "rounded" ? Math.min(20,Math.min(width,height)/2) : 0
        color: Qt.alpha(K.Theme.backgroundColor,.92)
    }
}
