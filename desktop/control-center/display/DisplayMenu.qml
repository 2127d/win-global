// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC
import org.kde.kirigami as Kirigami

QQC.Menu {
    id: menu
    popupType: QQC.Popup.Item
    padding: 8
    margins: 12
    overlap: 0
    implicitWidth: {
        let widest = 224;
        for (let i = 0; i < count; ++i) widest = Math.max(widest, itemAt(i)?.implicitWidth || 0);
        return Math.min(360,widest + leftPadding + rightPadding);
    }
    implicitHeight: contentItem.contentHeight + topPadding + bottomPadding
    width: Math.min(implicitWidth, Math.max(180,(parent?.Window.window?.width || 400) - 24))
    height: Math.min(implicitHeight, Math.max(80,(parent?.Window.window?.height || 600) - 24))
    delegate: DisplayMenuItem {}
    background: DisplayMenuSurface {}
    contentItem: ListView {
        implicitHeight: contentHeight
        model: menu.contentModel
        spacing: 2
        clip: true
        currentIndex: menu.currentIndex
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: true; keyNavigationWraps: true
        QQC.ScrollBar.vertical: QQC.ScrollBar { policy: QQC.ScrollBar.AsNeeded }
    }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.shortDuration } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.shortDuration } }
}
