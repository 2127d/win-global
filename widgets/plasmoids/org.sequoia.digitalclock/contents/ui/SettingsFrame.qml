// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

// Decorate only this applet's ConfigView. Keep Plasma's navigation, page loading,
// shortcuts, plugin settings and unsaved-change handling in charge of behavior.
Item {
    id: frame
    visible: false

    readonly property int inset: 8
    readonly property int gap: 12
    readonly property int contentInset: 8

    Component {
        id: roundedPage
        OpacityMask {
            id: mask
            objectName: "sequoiaClockSettingsPageMask"
            maskSource: Rectangle {
                width: mask.width
                height: mask.height
                radius: 14
                color: "white"
                antialiasing: true
            }
        }
    }

    Component {
        id: surface
        Kirigami.ShadowedRectangle {
            radius: 14
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
            shadow.size: 6
            shadow.yOffset: 2
            shadow.color: Qt.rgba(0, 0, 0, 0.16)
        }
    }

    function decorate() {
        if (Plasmoid.pluginName !== "org.sequoia.digitalclock"
                || typeof configDialog === "undefined") {
            return;
        }
        const host = configDialog.contentItem.children.find(child =>
            typeof child.applicationWindow === "function" && "globalConfigModel" in child);
        const app = host?.applicationWindow();
        const sidebar = host?.children.find(child => child instanceof QQC2.ScrollView);
        // The host is private Plasma API. Leave a changed shell intact rather
        // than partially restyling it when its expected structure is absent.
        if (!host || !app || !sidebar || !(app instanceof Kirigami.ApplicationItem)) {
            console.warn("Sequoia clock: unrecognized settings shell; keeping native frame");
            return;
        }

        host.objectName = "sequoiaClockSettings";
        // Paint the outer canvas once, using the same Window palette material
        // as Sequoia's decoration (including its alpha). Paint it as an item
        // so Qt composites premultiplied color correctly over a clear window.
        host.color = Qt.binding(() => host.Kirigami.Theme.backgroundColor);
        configDialog.color = "transparent";
        configDialog.objectName = "sequoiaDigitalClockSettings";
        host.Layout.minimumWidth += 2 * (inset + contentInset) + gap;
        host.Layout.minimumHeight += 2 * (inset + contentInset);
        for (const child of host.children) {
            if (child instanceof Kirigami.Separator) {
                child.visible = false;
            }
        }

        sidebar.objectName = "sequoiaClockSettingsSidebar";
        sidebar.z = 1;
        sidebar.anchors.margins = inset;
        sidebar.padding = 6;
        sidebar.background = surface.createObject(sidebar, {
            objectName: "sequoiaClockSettingsSidebarSurface"
        });

        const card = surface.createObject(host, {
            objectName: "sequoiaClockSettingsContentSurface",
            z: 0
        });
        card.anchors.top = host.top;
        card.anchors.bottom = host.bottom;
        card.anchors.left = sidebar.right;
        card.anchors.right = host.right;
        card.anchors.margins = inset;
        card.anchors.leftMargin = gap;

        app.anchors.left = sidebar.right;
        app.z = 1;
        app.anchors.margins = inset + contentInset;
        app.anchors.leftMargin = gap + contentInset;

        // Clip the page stack as a unit: native KCMs and their toolbars each
        // paint backgrounds. Rounding just the outer card leaves square layers.
        app.pageStack.layer.enabled = Qt.binding(() => app.pageStack.GraphicsInfo.api !== GraphicsInfo.Software);
        app.pageStack.layer.effect = roundedPage;
    }

    Component.onCompleted: Qt.callLater(decorate)
}
