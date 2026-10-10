// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

PlasmaCore.Dialog {
    id: popup
    property Item anchor
    property bool above: false
    property real menuWidth: 220
    property alias model: entries.model
    property int currentIndex: 0
    property bool restoreFocus: false
    signal triggered(int index)
    signal dismissed(bool restoreFocus)
    visible: false
    transientParent: anchor ? anchor.Window.window : null
    location: PlasmaCore.Types.Floating
    type: PlasmaCore.Dialog.PopupMenu
    hideOnWindowDeactivate: true
    // Dialog supplies the theme's rounded mask, shadow and native blur region.
    backgroundHints: PlasmaCore.Dialog.StandardBackground

    function reposition() {
        if (!anchor) return;
        const point = anchor.mapToGlobal(0, 0);
        x = Math.round(above ? point.x + anchor.width - width : point.x);
        y = Math.round(above ? point.y - height - 4 : point.y + anchor.height + 4);
    }
    function openAt(item, index) {
        anchor = item;
        currentIndex = Math.max(0, index);
        restoreFocus = false;
        reposition();
        visible = true;
        reposition();
        requestActivate();
        content.forceActiveFocus();
    }
    function closeMenu(returnFocus) {
        restoreFocus = returnFocus;
        if (returnFocus && anchor) anchor.forceActiveFocus();
        visible = false;
    }
    function choose(index) {
        closeMenu(true);
        triggered(index);
    }
    onWidthChanged: if (visible) reposition()
    onHeightChanged: if (visible) reposition()
    onVisibleChanged: if (!visible) dismissed(restoreFocus)

    mainItem: FocusScope {
        id: content
        width: Math.max(1, popup.menuWidth - popup.margins.left - popup.margins.right)
        height: entries.contentHeight + 8
        focus: true
        Keys.onEscapePressed: popup.closeMenu(true)
        Keys.onUpPressed: popup.currentIndex = (popup.currentIndex + entries.count - 1) % entries.count
        Keys.onDownPressed: popup.currentIndex = (popup.currentIndex + 1) % entries.count
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Home) { popup.currentIndex = 0; event.accepted = true; }
            else if (event.key === Qt.Key_End) { popup.currentIndex = entries.count - 1; event.accepted = true; }
        }
        Keys.onReturnPressed: if (entries.count) popup.choose(popup.currentIndex)
        Keys.onEnterPressed: if (entries.count) popup.choose(popup.currentIndex)
        Keys.onSpacePressed: if (entries.count) popup.choose(popup.currentIndex)
        Keys.onTabPressed: popup.closeMenu(true)
        ListView {
            id: entries
            anchors.fill: parent
            anchors.margins: 4
            interactive: false
            currentIndex: popup.currentIndex
            delegate: PC.AbstractButton {
                id: entry
                required property int index
                required property var model
                readonly property var entryData: model.display !== undefined ? model : popup.model[index]
                readonly property string iconName: entryData?.decoration || ""
                width: entries.width
                height: 34
                text: entryData?.display || ""
                hoverEnabled: true
                Accessible.name: text
                onHoveredChanged: if (hovered) popup.currentIndex = index
                onClicked: popup.choose(index)
                background: Rectangle {
                    radius: 6
                    color: entry.index === popup.currentIndex
                        ? Qt.alpha(Kirigami.Theme.highlightColor, 0.2) : "transparent"
                }
                contentItem: Item {
                    Kirigami.Icon {
                        id: icon
                        visible: !!entry.iconName
                        source: entry.iconName
                        width: 18; height: 18
                        x: 8
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: icon.visible ? 34 : 8
                        width: parent.width - x - 8
                        text: entry.text
                        color: Kirigami.Theme.textColor
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
