// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
import org.sequoia.settingsglass 1.0 as Glass
Item {
    id: root
    property string material: "glass"
    property bool miniature: false
    property string colorMode: "current"
    readonly property string resolvedMode: colorMode === "current" ? appearancePreview.colorMode : colorMode
    readonly property string renderKey: material + "/" + colorMode
    readonly property url renderedImage: desktopRenderer.images[renderKey] || ""
    readonly property string renderError: desktopRenderer.errors[renderKey] || ""
    // Frame the full image in landscape even on portrait-only workstations;
    // DesktopImage preserves its native proportions without stretching/cropping.
    readonly property real previewAspectRatio: Math.max(16 / 9, desktopRenderer.aspectRatio)
    implicitHeight: width / previewAspectRatio
    Layout.preferredHeight: implicitHeight
    Layout.minimumHeight: implicitHeight
    Layout.maximumHeight: implicitHeight
    function requestRender() { if (visible) desktopRenderer.request(material, colorMode, !miniature); }
    Component.onCompleted: Qt.callLater(requestRender)
    onRenderKeyChanged: Qt.callLater(requestRender)
    onVisibleChanged: if (visible) Qt.callLater(requestRender)
    Connections { target: desktopRenderer; function onInvalidated() { Qt.callLater(root.requestRender); } }
    Rectangle { anchors.fill: parent; radius: root.miniature ? 10 : 14; color: Qt.alpha(K.Theme.backgroundColor, .7) }
    Glass.DesktopImage {
        objectName: root.miniature ? "materialDesktopImage" : "actualDesktopImage"
        anchors.fill: parent
        source: root.renderedImage
        radius: root.miniature ? 10 : 14
        visible: source.toString().length > 0
        Accessible.name: qsTr("Rendered desktop with %1 appearance").arg(root.material)
    }
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, parent.width - 24)
        visible: root.renderedImage.toString().length === 0
        C.BusyIndicator { Layout.alignment: Qt.AlignHCenter; implicitWidth: root.miniature ? 22 : 30; implicitHeight: implicitWidth; running: parent.visible && !root.renderError.length }
        C.Label {
            Layout.fillWidth: true
            visible: !root.miniature || root.renderError.length > 0
            text: root.renderError.length ? (root.miniature ? qsTr("Unavailable") : root.renderError) : qsTr("Rendering your desktop…")
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
    }
}
