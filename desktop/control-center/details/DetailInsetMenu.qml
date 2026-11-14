// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

// This menu supplies its own surface and list. Desktop-style popup insets would
// otherwise size that surface separately from the rows painted on top of it.
T.Menu {
    id: menu
    property QtObject sequoiaLifetime: DetailPopupLifetime {
        popup: menu
        owner: menu.parent
    }
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
    width: Math.min(implicitWidth, Math.max(1, (parent?.Window.window?.width || 400) - 2 * margins))
    height: Math.min(implicitHeight, Math.max(1, (parent?.Window.window?.height || 600) - 2 * margins))
    contentItem: ListView {
        flickableDirection: Flickable.VerticalFlick
        implicitHeight: contentHeight
        model: menu.contentModel
        spacing: 2
        clip: true
        layer.enabled: interactive // Clip partially scrolled glass highlights on both Qt renderers.
        currentIndex: menu.currentIndex
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: true; keyNavigationWraps: true
        QQC.ScrollBar.vertical: DetailScrollBar {}
    }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.shortDuration } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Kirigami.Units.shortDuration } }
}
