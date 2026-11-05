// SPDX-License-Identifier: LGPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

ColumnLayout {
    id: section
    required property string title
    required property var model
    property bool showcase: false
    property int maximumColumns: showcase ? 3 : 4
    readonly property int columns: Math.max(1, Math.min(maximumColumns, Math.floor((width + 16) / (showcase ? 350 : 260))))
    readonly property int count: showcase ? shelf.count : gridRepeater.count
    signal showAll()
    signal revealRequested(var item)
    spacing: 12
    visible: count > 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        spacing: 10
        Kirigami.Heading {
            Layout.fillWidth: true
            text: section.title
            level: 2
            wrapMode: Text.Wrap
        }
        QQC2.AbstractButton {
            id: seeAll
            objectName: "sequoiaSeeAll"
            text: i18nc("@action:button", "See All")
            Accessible.name: i18nc("@action:button", "See all %1", section.title)
            focusPolicy: Qt.StrongFocus
            hoverEnabled: true
            padding: 5
            implicitWidth: implicitContentWidth + leftPadding + rightPadding
            implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
            background: Item {}
            contentItem: QQC2.Label {
                text: seeAll.text
                color: seeAll.Kirigami.Theme.linkColor
                font.underline: seeAll.hovered || seeAll.activeFocus
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: section.showAll()
            onActiveFocusChanged: if (activeFocus) section.revealRequested(seeAll)
        }
        QQC2.ToolButton {
            visible: section.showcase && shelf.count > section.columns
            enabled: shelf.currentIndex > 0
            icon.name: mirrored ? "go-next-symbolic" : "go-previous-symbolic"
            text: i18nc("@action:button", "Previous applications")
            display: QQC2.AbstractButton.IconOnly
            onClicked: shelf.move(-section.columns)
        }
        QQC2.ToolButton {
            visible: section.showcase && shelf.count > section.columns
            enabled: shelf.currentIndex + section.columns < shelf.count
            icon.name: mirrored ? "go-previous-symbolic" : "go-next-symbolic"
            text: i18nc("@action:button", "Next applications")
            display: QQC2.AbstractButton.IconOnly
            onClicked: shelf.move(section.columns)
        }
    }
    ListView {
        id: shelf
        objectName: "sequoiaHomeShelf"
        Layout.fillWidth: true
        Layout.preferredHeight: 264
        visible: section.showcase
        model: section.showcase ? section.model : null
        orientation: ListView.Horizontal
        spacing: 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
        currentIndex: 0
        keyNavigationEnabled: true
        highlightFollowsCurrentItem: false
        NumberAnimation {
            id: shelfSlide
            target: shelf
            property: "contentX"
            duration: Kirigami.Units.longDuration > 0 ? 300 : 0
            easing.type: Easing.OutCubic
        }
        function move(amount) {
            shelfSlide.stop()
            cancelFlick()
            const start = contentX
            currentIndex = Math.max(0, Math.min(count - 1, currentIndex + amount))
            // Let ListView resolve the destination, including RTL and the final
            // partial page, then animate there from the current visible position.
            positionViewAtIndex(currentIndex, ListView.Beginning)
            const destination = contentX
            contentX = start
            shelfSlide.from = start
            shelfSlide.to = destination
            shelfSlide.start()
        }
        function moveFocus(amount) {
            move(amount)
            Qt.callLater(() => currentItem?.forceActiveFocus(Qt.TabFocusReason))
        }
        onMovementEnded: {
            const first = indexAt(contentX + (effectiveLayoutDirection === Qt.RightToLeft ? width - 1 : 1), height / 2)
            if (first >= 0) currentIndex = first
        }
        onDraggingChanged: if (dragging) shelfSlide.stop()
        onWidthChanged: {
            shelfSlide.stop()
            Qt.callLater(() => positionViewAtIndex(currentIndex, ListView.Beginning))
        }
        onCountChanged: if (currentIndex >= count) currentIndex = Math.max(0, count - 1)
        delegate: FocusScope {
            id: tile
            required property int index
            required property var model
            width: shelf.width / section.columns
            height: shelf.height

            HomeAppCard {
                anchors.fill: parent
                anchors.margins: 8
                application: tile.model.application
                showcase: true
                focus: true
                imageHeight: Math.max(100, height - 84)
                onRevealRequested: item => {
                    shelf.currentIndex = tile.index
                    if (!shelfSlide.running) shelf.positionViewAtIndex(tile.index, ListView.Contain)
                    section.revealRequested(item)
                }
                Keys.onLeftPressed: shelf.moveFocus(Qt.application.layoutDirection === Qt.LeftToRight ? -1 : 1)
                Keys.onRightPressed: shelf.moveFocus(Qt.application.layoutDirection === Qt.LeftToRight ? 1 : -1)
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        visible: !section.showcase
        columns: section.columns
        rowSpacing: 16
        columnSpacing: 16
        Repeater {
            id: gridRepeater
            model: section.showcase ? null : section.model
            delegate: HomeAppCard {
                required property var model
                application: model.application
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.minimumWidth: 0
                onRevealRequested: item => section.revealRequested(item)
            }
        }
    }
}
