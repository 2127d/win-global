// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import Qt.labs.folderlistmodel
import org.kde.kirigami as Kirigami

FocusScope {
    id: fanRoot
    objectName: "downloadsFan"
    property url folderUrl
    property bool opened: true
    property real maximumHeight: 650
    property var entries: []
    readonly property int capacity: Math.max(1, Math.min(10, Math.floor((maximumHeight - 96) / 44)))
    readonly property int totalCount: downloadsModel.count
    readonly property bool loading: downloadsModel.status === FolderListModel.Loading
    implicitWidth: 320
    implicitHeight: Math.min(maximumHeight, Math.max(130, entries.length * 44 + 96))
    signal fileActivated(url fileUrl)
    signal folderRequested()
    signal dismissed()
    signal folded()
    property real reveal: 1
    Component.onCompleted: {
        revealAnimation.stop();
        reveal = opened ? 1 : 0;
    }
    readonly property real foldX: width / 2
    readonly property real foldY: height + 18
    readonly property int motionDuration: Math.round(Kirigami.Units.longDuration * (opened ? 2.6 : 2.2))
    function progress(index) {
        return Math.max(0, Math.min(1, reveal * 1.2 - index / Math.max(1, entries.length) * 0.2));
    }
    NumberAnimation {
        id: revealAnimation
        target: fanRoot
        property: "reveal"
        duration: fanRoot.motionDuration
        easing.type: Easing.InOutCubic
        onFinished: if (!fanRoot.opened && fanRoot.reveal < 0.001) fanRoot.folded()
    }
    Keys.onEscapePressed: dismissed()
    onOpenedChanged: {
        revealAnimation.stop();
        if (opened) { refresh(); Qt.callLater(focusNewest); }
        revealAnimation.to = opened ? 1 : 0;
        revealAnimation.start();
    }
    onCapacityChanged: refresh()

    function focusNewest() {
        if (fan.count) fan.itemAt(0).forceActiveFocus();
        else openFolder.forceActiveFocus();
    }
    function refresh() {
        const rows = [];
        for (let i = 0; i < Math.min(downloadsModel.count, capacity); ++i) {
            rows.push({name: downloadsModel.get(i, "fileName"), url: downloadsModel.get(i, "fileUrl"), directory: downloadsModel.get(i, "fileIsDir")});
        }
        entries = rows;
    }
    function iconFor(entry) {
        if (entry.directory) return "folder";
        const suffix = entry.name.split(".").pop().toLowerCase();
        if (suffix === "pdf") return "application-pdf";
        if (["zip", "gz", "xz", "7z", "rar", "tar"].includes(suffix)) return "application-x-compressed-tar";
        if (["png", "jpg", "jpeg", "webp", "gif", "svg"].includes(suffix)) return "image-x-generic";
        if (["mp3", "ogg", "flac", "wav", "m4a"].includes(suffix)) return "audio-x-generic";
        if (["mp4", "mkv", "mov", "webm"].includes(suffix)) return "video-x-generic";
        return "text-x-generic";
    }
    FolderListModel {
        id: downloadsModel
        objectName: "downloadsFiles"
        folder: fanRoot.folderUrl
        showDotAndDotDot: false
        showHidden: false
        sortField: FolderListModel.Time
        // Qt's Time order is newest first; reversing would put oldest first.
        sortReversed: false
        showDirsFirst: false
        onCountChanged: Qt.callLater(fanRoot.refresh)
        onStatusChanged: Qt.callLater(fanRoot.refresh)
    }
    Connections {
        target: downloadsModel
        function onDataChanged() { Qt.callLater(fanRoot.refresh); }
        function onModelReset() { Qt.callLater(fanRoot.refresh); }
        function onRowsInserted() { Qt.callLater(fanRoot.refresh); }
        function onRowsRemoved() { Qt.callLater(fanRoot.refresh); }
        function onLayoutChanged() { Qt.callLater(fanRoot.refresh); }
    }
    Repeater {
        id: fan
        model: fanRoot.entries
        delegate: FanEntry {
            id: fileButton
            required property int index
            required property var modelData
            objectName: "downloadEntry" + index
            readonly property real curve: index / Math.max(1, fanRoot.entries.length)
            readonly property real unfolding: fanRoot.progress(index)
            width: 232
            height: 44
            x: (fanRoot.foldX - 212) * (1 - unfolding) + Math.round(8 + 66 * curve * curve) * unfolding
            y: (fanRoot.foldY - 22) * (1 - unfolding) + (fanRoot.height - 50 - index * 44) * unfolding
            opacity: Math.min(1, unfolding * 5)
            tilt: -18 * (1 - unfolding) + curve * 15 * unfolding
            foldScale: 0.18 + 0.82 * unfolding
            labelReveal: Math.max(0, (unfolding - 0.3) / 0.7)
            enabled: fanRoot.opened
            text: modelData.name
            iconName: fanRoot.iconFor(modelData)
            onClicked: fanRoot.fileActivated(modelData.url)
            Keys.onUpPressed: { if (index + 1 < fan.count) fan.itemAt(index + 1).forceActiveFocus(); else openFolder.forceActiveFocus(); }
            Keys.onDownPressed: if (index > 0) fan.itemAt(index - 1).forceActiveFocus()
        }
    }
    Controls.Label {
        anchors.centerIn: parent
        visible: fanRoot.entries.length === 0
        opacity: fanRoot.reveal
        text: fanRoot.loading ? qsTr("Loading downloads…") : qsTr("No downloads yet")
        color: Kirigami.Theme.textColor
        padding: 8
        font: Kirigami.Theme.smallFont
        background: Rectangle { radius: 4; color: Kirigami.Theme.backgroundColor; opacity: 0.92 }
    }
    FanEntry {
        id: openFolder
        objectName: "openDownloadsFolder"
        width: 232
        height: 44
        readonly property real unfolding: fanRoot.progress(fanRoot.entries.length)
        x: (fanRoot.foldX - 212) * (1 - unfolding) + (fanRoot.entries.length ? 74 : 36) * unfolding
        y: (fanRoot.foldY - 22) * (1 - unfolding) + (fanRoot.entries.length ? fanRoot.height - 50 - fanRoot.entries.length * 44 : 2) * unfolding
        tilt: -18 * (1 - unfolding) + (fanRoot.entries.length ? 15 : 0) * unfolding
        foldScale: 0.18 + 0.82 * unfolding
        labelReveal: Math.max(0, (unfolding - 0.3) / 0.7)
        opacity: Math.min(1, unfolding * 5)
        enabled: fanRoot.opened
        text: qsTr("See more in File Manager")
        iconName: "go-next"
        onClicked: fanRoot.folderRequested()
        Keys.onDownPressed: if (fan.count) fan.itemAt(fan.count - 1).forceActiveFocus()
    }
}
