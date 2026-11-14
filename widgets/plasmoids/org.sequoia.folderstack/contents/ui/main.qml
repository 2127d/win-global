// SPDX-License-Identifier: GPL-2.0-or-later
// Folder Stack by XxKrustyxX; Sequoia motion integration.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtCore
import Qt.labs.folderlistmodel
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.extras as PlasmaExtras
import org.kde.draganddrop as DragDrop
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    objectName: "sequoiaFolderStack"
    property bool closing: false
    readonly property int openDuration: Math.round(Kirigami.Units.longDuration * 2.6)
    readonly property int closeDuration: Math.round(Kirigami.Units.longDuration * 2.2)
    function rowProgress(index, count, grow) {
        return Math.max(0, Math.min(1, progress * grow * 1.2 - index / Math.max(1, count) * 0.2));
    }

    // Empty setting = the Downloads folder, same as a fresh macOS dock
    readonly property url folderUrl: {
        const f = Plasmoid.configuration.folder;
        if (!f)
            return StandardPaths.writableLocation(StandardPaths.DownloadLocation);
        return f.startsWith("file:") ? f : "file://" + f;
    }
    readonly property string folderName: recentMode ? i18n("Recent Files")
                                                    : decodeURIComponent(folderUrl.toString().replace(/\/$/, "").split("/").pop())
    // Show the files used most recently in any folder instead of a folder (see contents/code/recent-files.py)
    readonly property bool recentMode: Plasmoid.configuration.recentFiles
    property var recentEntries: []   // kept fresh in the background so the stack opens with them straight away
    readonly property int maxItems: Math.max(1, Plasmoid.configuration.maxItems)

    // Which edge the dock is on. The fan climbs away from a bottom dock and hangs down from a top one;
    // on a dock down the side of the screen (like macOS) the stack always opens as the grid.
    readonly property bool dockOnTop: Plasmoid.location === PlasmaCore.Types.TopEdge
    readonly property bool dockOnLeft: Plasmoid.location === PlasmaCore.Types.LeftEdge
    readonly property bool dockOnRight: Plasmoid.location === PlasmaCore.Types.RightEdge
    readonly property bool fanAllowed: !dockOnLeft && !dockOnRight

    // Fan geometry
    readonly property int iconSize: Kirigami.Units.iconSizes.large
    readonly property int rowHeight: iconSize + Kirigami.Units.smallSpacing * 3
    readonly property real radius: 2000
    readonly property int labelWidth: Kirigami.Units.gridUnit * 13

    property var allEntries: []    // snapshot of the folder taken each time the stack opens
    // Open as: 0 = fan, 1 = grid, 2 = automatic (fan unless there are more files than fit)
    property bool showGrid: false

    // Subfolders opened inside the grid, innermost last, as { name, url }. Empty = the stack's own folder.
    property var browsePath: []
    property var subEntries: []    // contents of the innermost one
    property var parentLists: []   // contents of each folder behind the innermost one, outermost first
    onSubEntriesChanged: requestThumbs(subEntries)
    readonly property bool browsing: browsePath.length > 0
    readonly property var gridEntries: browsing ? subEntries : allEntries
    readonly property url gridFolderUrl: browsing ? browsePath[browsePath.length - 1].url : folderUrl
    readonly property string gridFolderName: browsing ? browsePath[browsePath.length - 1].name : folderName
    // Subfolders in the grid: 0 = open in place with a back button, 1 = stack on top of the folder they came from
    readonly property bool stackedFolders: Plasmoid.configuration.subfolderMode === 1
    // Fans on screen, oldest first: just the one, or with "Stacked on top" one per folder opened
    readonly property var fanLevels: (stackedFolders ? parentLists : []).concat([gridEntries])
    property real pushProgress: 1  // 0 → 1 as a newly stacked folder slides into place
    property real progress: 0      // 0 = tucked into the dock, 1 = fully fanned out
    property double lastClosed: 0
    property bool dragging: false  // a file is being dragged out; keep the popup alive until it's dropped
    property bool menuOpen: false  // the right-click menu takes the focus; keep the popup alive while it's up

    // The dock icon is shown as-is; the fan is our own popup, not a standard expanded view
    preferredRepresentation: fullRepresentation

    FolderListModel {
        id: folderModel
        folder: root.folderUrl
        showDirs: true
        showDirsFirst: false
        showDotAndDotDot: false
        showHidden: false
        sortField: Plasmoid.configuration.sortMode === 1 ? FolderListModel.Name : FolderListModel.Time
    }

    function folderIcon() {
        if (recentMode)
            return "document-open-recent";
        switch (folderName) {
        case "Downloads": return "folder-download";
        case "Documents": return "folder-documents";
        case "Pictures": return "folder-pictures";
        case "Music": return "folder-music";
        case "Videos": return "folder-videos";
        case "Desktop": return "user-desktop";
        default: return "folder";
        }
    }

    function isImage(suffix) {
        return ["png", "jpg", "jpeg", "gif", "webp", "bmp", "svg", "avif", "heic"].indexOf(suffix) >= 0;
    }

    function iconFor(entry) {
        if (entry.isDir)
            return "folder";
        const s = entry.suffix;
        const map = {
            "pdf": "application-pdf",
            "png": "image-x-generic", "jpg": "image-x-generic", "jpeg": "image-x-generic", "gif": "image-x-generic",
            "webp": "image-x-generic", "bmp": "image-x-generic", "svg": "image-x-generic", "avif": "image-x-generic",
            "heic": "image-x-generic",
            "doc": "x-office-document", "docx": "x-office-document", "odt": "x-office-document", "rtf": "x-office-document",
            "xls": "x-office-spreadsheet", "xlsx": "x-office-spreadsheet", "ods": "x-office-spreadsheet", "csv": "x-office-spreadsheet",
            "ppt": "x-office-presentation", "pptx": "x-office-presentation", "odp": "x-office-presentation",
            "zip": "package-x-generic", "tar": "package-x-generic", "gz": "package-x-generic", "xz": "package-x-generic",
            "7z": "package-x-generic", "rar": "package-x-generic", "bz2": "package-x-generic", "zst": "package-x-generic",
            "exe": "application-x-ms-dos-executable", "msi": "application-x-ms-dos-executable",
            "iso": "application-x-cd-image", "img": "application-x-cd-image", "dmg": "application-x-cd-image",
            "rpm": "application-x-rpm", "deb": "application-x-deb", "appimage": "application-x-executable",
            "mp3": "audio-x-generic", "wav": "audio-x-generic", "flac": "audio-x-generic", "ogg": "audio-x-generic", "m4a": "audio-x-generic",
            "mp4": "video-x-generic", "mkv": "video-x-generic", "mov": "video-x-generic", "webm": "video-x-generic", "avi": "video-x-generic",
            "txt": "text-x-generic", "md": "text-x-generic", "log": "text-x-generic",
            "html": "text-html", "htm": "text-html", "json": "application-json",
            "py": "text-x-python", "sh": "application-x-shellscript"
        };
        return map[s] || "unknown";
    }

    // Endings browsers give a file while it's still downloading (Firefox, Chrome/Edge/Brave, Opera)
    readonly property var partialEndings: ["part", "crdownload", "opdownload"]

    function lastSuffix(name) {
        const dot = name.lastIndexOf(".");
        return dot > 0 ? name.slice(dot + 1).toLowerCase() : "";
    }

    // The real names of the downloads still in progress in a folder
    function downloadsIn(model) {
        const names = [];
        for (let i = 0; i < model.count; i++) {
            const name = model.get(i, "fileName");
            if (partialEndings.indexOf(lastSuffix(name)) >= 0)
                names.push(name.slice(0, name.lastIndexOf(".")));
        }
        return names;
    }

    function readFolder(model) {
        const downloading = downloadsIn(model);
        const list = [];
        for (let i = 0; i < model.count; i++) {
            let name = model.get(i, "fileName");
            const isDir = model.get(i, "fileIsDir");
            const partial = !isDir && partialEndings.indexOf(lastSuffix(name)) >= 0;
            if (partial)
                name = name.slice(0, name.lastIndexOf("."));   // show "report.pdf", not "report.pdf.part"
            else if (!isDir && downloading.indexOf(name) >= 0)
                continue;   // Firefox's empty placeholder for a download in progress
            list.push({
                name: name,
                url: model.get(i, "fileUrl"),
                path: model.get(i, "filePath"),
                isDir: isDir,
                suffix: lastSuffix(name),   // "tar.gz" -> "gz"
                size: model.get(i, "fileSize"),
                downloading: partial
            });
        }
        return list;
    }

    // ---- Previews (PDF pages, video frames, album art, office documents), made by contents/code/thumbs.py.
    // Ones Dolphin has already made are reused; the rest are made in the background the first time they're shown.
    property var thumbs: ({})      // file url -> preview url
    property var thumbJobs: ({})   // job id -> urls asked about
    readonly property var previewSuffixes: ["pdf", "mp4", "mkv", "mov", "webm", "avi", "m4v", "wmv", "mpg", "mpeg",
        "mp3", "flac", "m4a", "ogg", "opus", "aac", "wav",
        "odt", "ods", "odp", "odg", "ott", "ots", "otp", "doc", "docx", "xls", "xlsx", "ppt", "pptx"]

    function requestThumbs(list) {
        if (!Plasmoid.configuration.showPreviews)
            return;
        const urls = list.filter(e => !e.isDir && !e.downloading && previewSuffixes.indexOf(e.suffix) >= 0
                                      && !(e.url.toString() in thumbs))
            .slice(0, 300)
            .map(e => e.url.toString());
        if (urls.length > 0)
            runThumbs("lookup", urls);
    }

    function runThumbs(mode, urls) {
        const id = `${mode}-${Date.now()}-${Math.floor(Math.random() * 1e6)}`;
        thumbJobs[id] = urls;
        const args = urls.map(u => shellQuote(u)).join(" ");
        runner.connectSource(`python3 -I ${shellQuote(scriptPath("thumbs.py"))} ${mode} ${args} # thumbs ${id}`);
    }

    function thumbsDone(id, stdout) {
        const found = Object.assign({}, thumbs);
        for (const line of stdout.split("\n")) {
            const tab = line.indexOf("\t");
            if (tab > 0)
                found[line.slice(0, tab)] = line.slice(tab + 1);
        }
        thumbs = found;
        const asked = thumbJobs[id] || [];
        delete thumbJobs[id];
        // Anything the quick look-up didn't find gets made now
        if (id.startsWith("lookup-")) {
            const missing = asked.filter(u => !(u in found));
            if (missing.length > 0)
                runThumbs("make", missing);
        }
    }

    // ---- Downloads: the dock icon shows a bar while something is downloading and bounces when it finishes
    signal downloadFinished()
    property var activeDownloads: []   // real names of downloads in progress in the stack's folder
    property var downloadSizes: ({})   // path of a download in progress -> bytes so far

    // ---- New files: a dot on anything added since the stack was last closed, and a count on the dock icon
    property var newPaths: []

    function checkNew() {
        if (!Plasmoid.configuration.markNew || recentMode) {
            newPaths = [];
            return;
        }
        const since = Number(Plasmoid.configuration.lastSeen);
        if (!since) {
            // First run: start counting from now rather than calling everything new
            Plasmoid.configuration.lastSeen = String(Math.floor(Date.now() / 1000));
            return;
        }
        runner.connectSource(`bash ${shellQuote(scriptPath("new-files.sh"))} ${shellQuote(folderPath())} ${since} # new ${Date.now()}`);
    }

    function newFound(stdout) {
        const dir = folderPath().replace(/\/$/, "");
        newPaths = stdout.split("\n").filter(name => name !== "" && !name.startsWith(".")
                                              && partialEndings.indexOf(lastSuffix(name)) < 0
                                              && activeDownloads.indexOf(name) < 0)   // still downloading
            .map(name => dir + "/" + name);
    }

    function folderPath() {
        return decodeURIComponent(folderUrl.toString().replace(/^file:\/\//, ""));
    }

    Connections {
        target: Plasmoid.configuration
        function onMarkNewChanged() { root.checkNew() }
        // A different folder starts with nothing new (only a real change of the setting:
        // folderUrl itself also "changes" while Plasma starts up)
        function onFolderChanged() { Plasmoid.configuration.lastSeen = String(Math.floor(Date.now() / 1000)) }
    }

    function folderChanged() {
        if (recentMode) {
            // The folder setting is kept but not watched
            activeDownloads = [];
            newPaths = [];
            return;
        }
        const now = downloadsIn(folderModel);
        const names = [];
        for (let i = 0; i < folderModel.count; i++)
            names.push(folderModel.get(i, "fileName"));
        // Gone from the in-progress list and the finished file is there = a download just finished
        const finished = activeDownloads.some(n => now.indexOf(n) < 0 && names.indexOf(n) >= 0);
        activeDownloads = now;
        checkNew();
        if (finished && Plasmoid.configuration.bounceOnDownload)
            downloadFinished();
        // Keep an open stack up to date (a download finishing turns into a normal file)
        if (popupOpen && !browsing) {
            allEntries = readFolder(folderModel);
            requestThumbs(allEntries);
        }
    }

    Connections {
        target: folderModel
        function onRowsInserted() { Qt.callLater(root.folderChanged) }
        function onRowsRemoved() { Qt.callLater(root.folderChanged) }
        function onDataChanged() { Qt.callLater(root.folderChanged) }   // renames like "x.crdownload" -> "x"
        function onModelReset() { Qt.callLater(root.folderChanged) }
        function onStatusChanged() { Qt.callLater(root.folderChanged) }
    }

    function sizeText(entry) {
        const bytes = entry.path in downloadSizes ? downloadSizes[entry.path] : entry.size;
        return Qt.locale().formattedDataSize(bytes, 1);
    }

    // While an open stack shows downloads in progress, update how much has come in every second
    Timer {
        interval: 1000
        repeat: true
        running: root.popupOpen && root.gridEntries.some(e => e.downloading)
        onTriggered: {
            const paths = root.gridEntries.filter(e => e.downloading).map(e => root.shellQuote(e.path)).join(" ");
            runner.connectSource(`stat -c '%s %n' -- ${paths} # download-sizes ${Date.now()}`);
        }
    }

    // ---- Recent files: the list KDE (and GTK apps) keep of files opened or saved, newest first
    function refreshRecent() {
        if (recentMode)
            runner.connectSource(`python3 -I ${shellQuote(scriptPath("recent-files.py"))} 100 # recent ${Date.now()}`);
    }

    function recentFound(stdout) {
        const list = [];
        for (const line of stdout.split("\n")) {
            const parts = line.split("\t");
            if (parts.length < 3)
                continue;
            const path = parts.slice(2).join("\t");
            const name = path.slice(path.lastIndexOf("/") + 1);
            list.push({
                name: name,
                url: "file://" + path.split("/").map(encodeURIComponent).join("/"),
                path: path,
                isDir: false,
                suffix: lastSuffix(name),
                size: Number(parts[1]),
                downloading: false
            });
        }
        if (Plasmoid.configuration.sortMode === 1)
            list.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: "base" }));
        recentEntries = list;
        if (popupOpen && !browsing) {
            allEntries = list;
            requestThumbs(list);
        }
    }

    onRecentModeChanged: {
        recentEntries = [];
        refreshRecent();
        folderChanged();
    }

    // Opening a file anywhere doesn't tell us, so look again now and then (and every time the stack opens)
    Timer {
        interval: 120000
        repeat: true
        running: root.recentMode && !root.popupOpen
        onTriggered: root.refreshRecent()
    }

    Connections {
        target: Plasmoid.configuration
        function onSortModeChanged() { root.refreshRecent() }
    }

    // Recent Files can only be shown by file managers that understand KDE's recentlyused:/ address
    readonly property bool canOpenStackFolder: !recentMode
        || ["org.kde.dolphin.desktop", "org.kde.konqueror.desktop", "org.kde.krusader.desktop"].indexOf(fileManagerId) >= 0

    // The stack's own place in the file manager: the folder, or Recent Files
    function openStackFolder(url) {
        if (recentMode && !browsing) {
            closeFan();
            fileAction(`launch ${shellQuote(fileManagerId)} recentlyused:/files/`);
        } else {
            openUrl(url);
        }
    }

    function openFan() {
        closeAnim.stop();
        closing = false;
        const list = recentMode ? recentEntries : readFolder(folderModel);
        refreshRecent();
        lookUpFileManager();
        browsePath = [];
        subEntries = [];
        parentLists = [];
        const mode = Plasmoid.configuration.viewMode;
        showGrid = !fanAllowed || mode === 1 || (mode === 2 && list.length > maxItems);
        allEntries = list;
        requestThumbs(list);
        progress = 0;
        fanLoader.active = true;
        fan.visualParent = dockIcon;
        fan.visible = true;
        fan.requestActivate();
        // Let the native surface map before the first animation frame.
        Qt.callLater(() => { if (popupOpen && !closing) openAnim.restart(); });
    }

    function enterFolder(entry) {
        parentLists = parentLists.concat([gridEntries]);
        subEntries = [];
        browsePath = browsePath.concat([{ name: entry.name, url: entry.url }]);
        if (stackedFolders)
            pushAnim.restart();
    }

    // Closes every folder stacked in front of the given level (0 = the stack's own folder)
    function backTo(level) {
        parentLists = parentLists.slice(0, level);
        subEntries = [];
        browsePath = browsePath.slice(0, level);
    }

    NumberAnimation {
        id: pushAnim
        target: root
        property: "pushProgress"
        from: 0; to: 1
        duration: 180
        easing.type: Easing.OutCubic
    }

    function goBack() {
        parentLists = parentLists.slice(0, -1);
        subEntries = [];
        browsePath = browsePath.slice(0, -1);
    }

    // Reads whichever subfolder the grid is showing
    FolderListModel {
        id: browseModel
        folder: root.browsing ? root.gridFolderUrl : ""
        showDirs: true
        showDirsFirst: false
        showDotAndDotDot: false
        showHidden: false
        sortField: folderModel.sortField
        onStatusChanged: if (status === FolderListModel.Ready && root.browsing) root.subEntries = root.readFolder(browseModel)
        // (previews for a subfolder are asked for in onSubEntriesChanged)
        onCountChanged: if (status === FolderListModel.Ready && root.browsing) root.subEntries = root.readFolder(browseModel)
    }

    // Renames keep the count the same, so catch those too
    Connections {
        target: browseModel
        function onDataChanged() { if (root.browsing) root.subEntries = root.readFolder(browseModel) }
        function onModelReset() { if (root.browsing && browseModel.status === FolderListModel.Ready) root.subEntries = root.readFolder(browseModel) }
    }

    function closeFan() {
        if (!popupOpen || closing) return;
        openAnim.stop();
        closing = true;
        lastClosed = Date.now();
        closeAnim.restart();
    }

    function shellQuote(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'";
    }

    property var droppedUrls: []   // waiting for a choice from the Move / Copy menu

    // Files dropped on the dock icon go into the folder (see contents/code/drop.sh). mode is "move" or "copy".
    // Path of one of our helper scripts in contents/code
    function scriptPath(name) {
        return decodeURIComponent(Qt.resolvedUrl("../code/" + name).toString().replace(/^file:\/\//, ""));
    }

    // dest: the folder (a path) the files go into
    function receive(urls, mode, dest) {
        const script = scriptPath("drop.sh");
        const args = Array.from(urls, u => shellQuote(u.toString())).join(" ");
        // The timestamp keeps two identical drops from looking like the same command to the runner
        runner.connectSource(`bash ${shellQuote(script)} ${mode} ${shellQuote(dest)} ${args} # ${Date.now()}`);
    }

    // The dock panel opens a "new widget goes here" gap while files are dragged over it, and only
    // closes it when the drag leaves the panel or lands on the panel itself. Files that land on us
    // never tell it, so close the gap ourselves by giving the panel's drop area its "drag left" signal.
    function closePanelGap() {
        for (let item = root.parent; item; item = item.parent) {
            if (item.marginAreasEnabled !== undefined && typeof item.dragLeave === "function") {
                item.dragLeave(null);
                return;
            }
        }
    }

    // Like dropping onto a folder in Dolphin: Shift moves, Ctrl copies, otherwise ask
    // dest: the folder (a path) they go into. menuAt: where to ask Move or Copy, { item, x, y } inside
    // the open stack; without it the menu opens by the dock icon.
    function handleDrop(urls, modifiers, dest, menuAt) {
        const inFolder = "file://" + dest.replace(/\/$/, "") + "/";
        // Files dragged out of this folder and straight back in are already where they belong,
        // and a folder can't go inside itself
        const incoming = Array.from(urls, u => u.toString())
            .filter(u => !(u.startsWith(inFolder) && u.slice(inFolder.length).indexOf("/") < 0))
            .filter(u => u.replace(/\/$/, "") !== "file://" + dest.replace(/\/$/, ""));
        if (incoming.length === 0)
            return false;
        if (modifiers & Qt.ShiftModifier) {
            receive(incoming, "move", dest);
        } else if (modifiers & Qt.ControlModifier) {
            receive(incoming, "copy", dest);
        } else {
            droppedUrls = incoming;
            dropDest = dest;
            moveItem.enabled = incoming.every(u => u.startsWith("file:"));   // web links can only be downloaded
            // Opened after the drop has finished, so the drag doesn't swallow the menu
            Qt.callLater(() => {
                if (menuAt && popupOpen) {
                    menuOpen = true;   // keep the stack up behind it
                    dropMenu.visualParent = menuAt.item;
                    dropMenu.transientParent = fan;
                    dropMenu.open(menuAt.x, menuAt.y);
                } else {
                    dropMenu.visualParent = dockIcon;
                    dropMenu.transientParent = null;
                    dropMenu.openRelative();
                }
            });
            return true;
        }
        return false;
    }
    property string dropDest: ""

    // ---- Drag files onto the dock icon and hold: the stack opens, folders in it open when held over,
    // and the files can be dropped into any of them (macOS calls this spring-loading)
    property bool springLoaded: false     // opened by holding a drag over the dock icon
    property bool dragOverDock: false
    property bool dragOverPopup: false
    property var dropHover: null          // what a drag over the open stack is pointing at (see dropTargetAt)

    Timer {
        id: springOpen
        interval: 700
        onTriggered: {
            if (root.dragOverDock && !root.popupOpen) {
                root.openFan();
                root.springLoaded = true;
            }
        }
    }

    // Opens the folder held over (or goes back to the folder behind)
    Timer {
        id: springFolder
        interval: 800
        onTriggered: {
            const t = root.dropHover;
            if (!t || !root.popupOpen)
                return;
            if (t.level !== undefined)
                root.backTo(t.level);
            else if (root.showGrid || root.stackedFolders)
                root.enterFolder(t.entry);
            root.dropHover = null;
        }
    }

    // A drag that wanders off both the dock icon and the stack closes the stack it opened
    Timer {
        id: springClose
        interval: 600
        onTriggered: {
            if (root.springLoaded && !root.dragOverDock && !root.dragOverPopup && !root.menuOpen)
                root.closeFan();
        }
    }

    function dragPlaceChanged() {
        if (!dragOverDock && !dragOverPopup)
            springClose.restart();
        else
            springClose.stop();
    }
    onDragOverDockChanged: dragPlaceChanged()
    onDragOverPopupChanged: dragPlaceChanged()

    // What's under a point in the open stack: a folder ({ entry }), a folder behind the open one
    // ({ level }), or null for the folder on show. Delegates say what they are with a dropTarget property.
    function dropTargetAt(item, x, y) {
        // Front to back: the fans and boxes overlap, and each covers the whole popup
        const kids = Array.from(item.children).map((c, i) => ({ c: c, i: i }))
            .sort((a, b) => (b.c.z - a.c.z) || (b.i - a.i));
        for (const { c } of kids) {
            if (!c.visible)
                continue;
            const p = item.mapToItem(c, x, y);
            if (!c.contains(p))
                continue;
            if (c.dropTarget !== undefined) {
                if (c.dropTarget !== null)
                    return c.dropTarget;
                continue;   // something that isn't a folder: look behind it
            }
            const found = dropTargetAt(c, p.x, p.y);
            if (found)
                return found;
            // The grid box hides the stacked folders behind it
            if (c.dropOpaque === true)
                return null;
        }
        return null;
    }

    function sameTarget(a, b) {
        if (!a || !b)
            return a === b;
        return a.level === b.level && (a.entry ? a.entry.path : "") === (b.entry ? b.entry.path : "");
    }

    function dragMovedOver(body, x, y) {
        const t = dropTargetAt(body, x, y);
        if (sameTarget(t, dropHover))
            return;
        dropHover = t;
        // Folders spring open in the grid, or in the fan when folders stack on top
        if (t && (t.level !== undefined || showGrid || stackedFolders))
            springFolder.restart();
        else
            springFolder.stop();
    }

    function folderPathOf(level) {
        const url = level > 0 && browsePath[level - 1] ? browsePath[level - 1].url : folderUrl;
        return decodeURIComponent(url.toString().replace(/^file:\/\//, ""));
    }

    function droppedOnStack(body, event) {
        const t = dropTargetAt(body, event.x, event.y);
        springFolder.stop();
        dropHover = null;
        dragOverPopup = false;
        const dest = t && t.entry ? t.entry.path
            : t && t.level !== undefined ? folderPathOf(t.level)
            : decodeURIComponent(gridFolderUrl.toString().replace(/^file:\/\//, ""));
        const asking = handleDrop(event.mimeData.urls, event.modifiers, dest, { item: body, x: event.x, y: event.y });
        // Done with the stack, unless the Move / Copy menu still needs it
        if (!asking && springLoaded)
            closeFan();
    }

    PlasmaExtras.Menu {
        id: dropMenu

        onStatusChanged: {
            if (status === PlasmaExtras.Menu.Closed && root.menuOpen)
                Qt.callLater(() => {
                    root.menuOpen = false;
                    // Dropped into the stack: it's done its job
                    if (root.popupOpen)
                        root.closeFan();
                });
        }

        PlasmaExtras.MenuItem {
            id: moveItem
            text: i18n("&Move Here")
            icon: "go-jump"
            onClicked: root.receive(root.droppedUrls, "move", root.dropDest)
        }
        PlasmaExtras.MenuItem {
            text: i18n("&Copy Here")
            icon: "edit-copy"
            onClicked: root.receive(root.droppedUrls, "copy", root.dropDest)
        }
        PlasmaExtras.MenuItem {
            separator: true
        }
        PlasmaExtras.MenuItem {
            text: i18n("C&ancel")
            icon: "process-stop"
        }
    }

    // ---- Selecting several files: Ctrl-click adds or removes one, Shift-click takes everything in between,
    // Ctrl+A takes them all. Dragging or right-clicking a selected file then works on the whole selection.
    property var selection: []        // paths of the selected files, in the folder on show
    property string selectionAnchor: ""   // where a Shift-click range starts

    // The files that can be picked right now: the open grid, or the front fan
    function selectableList() {
        return (showGrid ? shownEntries : gridEntries.slice(0, maxItems)).filter(e => !e.downloading);
    }

    // ---- Type to filter: typing while the stack is open shows only the files whose names contain the text
    property string filterText: ""
    readonly property var shownEntries: {
        const words = filterText.trim().toLowerCase();
        return words === "" ? gridEntries : gridEntries.filter(e => e.name.toLowerCase().indexOf(words) >= 0);
    }
    // Each folder starts unfiltered
    onGridFolderUrlChanged: filterText = ""

    function clearSelection() {
        selection = [];
        selectionAnchor = "";
    }

    // Handles Ctrl/Shift clicks; returns false for a plain click, which then opens the file as usual
    function selectClick(entry, modifiers) {
        if (entry === null || entry.downloading || !(modifiers & (Qt.ControlModifier | Qt.ShiftModifier)))
            return false;
        const list = selectableList().map(e => e.path);
        if (modifiers & Qt.ShiftModifier) {
            const from = list.indexOf(selectionAnchor) >= 0 ? list.indexOf(selectionAnchor) : list.indexOf(entry.path);
            const to = list.indexOf(entry.path);
            const range = list.slice(Math.min(from, to), Math.max(from, to) + 1);
            // Ctrl+Shift adds the range to what's already picked
            selection = (modifiers & Qt.ControlModifier) ? selection.concat(range.filter(p => selection.indexOf(p) < 0)) : range;
            if (selectionAnchor === "")
                selectionAnchor = entry.path;
        } else {
            selection = selection.indexOf(entry.path) >= 0 ? selection.filter(p => p !== entry.path) : selection.concat([entry.path]);
            selectionAnchor = entry.path;
        }
        return true;
    }

    function selectAll() {
        selection = selectableList().map(e => e.path);
    }

    // What dragging or right-clicking this file acts on: the whole selection if it's part of it, else just it
    function targetsFor(entry) {
        if (selection.length > 1 && selection.indexOf(entry.path) >= 0)
            return selectableList().filter(e => selection.indexOf(e.path) >= 0);
        return [entry];
    }

    // Files that go (binned, renamed, another folder opened) drop out of the selection
    onGridEntriesChanged: selection = selection.filter(p => gridEntries.some(e => e.path === p))

    // ---- Right-click menu on a file or folder in the stack (see contents/code/file-actions.sh)
    property var menuEntry: null       // the file right-clicked
    property var menuEntries: []       // everything the menu acts on (the selection, or just that file)
    readonly property bool menuMany: menuEntries.length > 1
    property bool menuInGrid: false
    property var openWithApps: []   // [{ id, name, icon }], the default app first
    property string appsJob: ""

    function quotedUrls(entries) {
        return entries.map(e => shellQuote(e.url)).join(" ");
    }

    function showFileMenu(entry, item, x, y, inGrid) {
        if (entry === null || entry.downloading)
            return;
        menuEntries = targetsFor(entry);
        // Right-clicking something outside the selection drops it, like Dolphin
        if (menuEntries.length === 1 && selection.length > 0 && selection.indexOf(entry.path) < 0)
            clearSelection();
        menuEntry = entry;
        menuInGrid = inGrid;
        fileMenu.visualParent = item;
        fileMenu.openAt = Qt.point(x, y);
        appsJob = `${Date.now()}`;
        // Files and folders together have no app in common
        if (menuMany && menuEntries.some(e => e.isDir)) {
            openWithApps = [];
            openFileMenu();
            return;
        }
        // Ask which apps open it, then show the menu (looking them up takes a few milliseconds)
        runner.connectSource(`python3 -I ${shellQuote(scriptPath("apps.py"))} ${shellQuote(entry.path)} # apps ${appsJob}`);
        appsTimeout.restart();
    }

    function appsFound(job, stdout) {
        if (job !== appsJob)
            return;
        openWithApps = stdout.split("\n").filter(l => l.indexOf("\t") > 0).map(l => {
            const [id, name, icon] = l.split("\t");
            return { id: id, name: name, icon: icon };
        });
        openFileMenu();
    }

    // If the look-up is slow, show the menu without its apps rather than not at all
    Timer {
        id: appsTimeout
        interval: 400
        onTriggered: {
            root.openWithApps = [];
            root.openFileMenu();
        }
    }

    function openFileMenu() {
        appsTimeout.stop();
        if (appsJob === "" || !popupOpen)
            return;
        appsJob = "";
        menuOpen = true;
        // Belongs to the stack's own window, so it opens on top of it and in the right place
        fileMenu.transientParent = fan;
        fileMenu.open(fileMenu.openAt.x, fileMenu.openAt.y);
    }

    function fileAction(args) {
        runner.connectSource(`bash ${shellQuote(scriptPath("file-actions.sh"))} ${args} # action ${Date.now()}`);
    }

    // What a left click does with a folder: open it in the stack, or in the file manager
    function openEntry(entry, inGrid) {
        if (entry.isDir && (inGrid || stackedFolders))
            enterFolder(entry);
        else
            openUrl(entry.url);
    }

    // ---- Renaming, right in the fan or grid like Dolphin
    property string renamingPath: ""   // file being renamed; empty = none
    property string renameText: ""
    property string renameError: ""
    property string renameJob: ""

    function startRename(entry) {
        renameError = "";
        renameText = entry.name;
        renamingPath = entry.path;
    }

    function cancelRename() {
        renamingPath = "";
        renameError = "";
    }

    function finishRename() {
        if (renamingPath === "")
            return;
        const oldName = renamingPath.slice(renamingPath.lastIndexOf("/") + 1);
        const newName = renameText.trim();
        if (newName === "" || newName === oldName) {
            cancelRename();
        } else if (newName.indexOf("/") >= 0) {
            renameError = i18n("A name can't contain “/”");
        } else if (newName === "." || newName === "..") {
            renameError = i18n("That name isn't allowed");
        } else {
            renameJob = `${Date.now()}`;
            runner.connectSource(`bash ${shellQuote(scriptPath("file-actions.sh"))} rename ${shellQuote(renamingPath)} ${shellQuote(newName)} # rename ${renameJob}`);
        }
    }

    function renameDone(job, stdout, exitCode) {
        if (job !== renameJob)
            return;
        if (stdout.trim() === "exists")
            renameError = i18n("Something called “%1” is already here", renameText.trim());
        else if (exitCode !== 0)
            renameError = i18n("Couldn't rename it");
        else
            cancelRename();
    }

    // Starts each time the stack opens with nothing being renamed, selected or filtered
    onPopupOpenChanged: {
        if (!popupOpen) {
            cancelRename();
            clearSelection();
            filterText = "";
            springLoaded = false;
            dragOverPopup = false;
            dropHover = null;
            // Everything has been seen now
            Plasmoid.configuration.lastSeen = String(Math.floor(Date.now() / 1000));
            newPaths = [];
        }
    }

    PlasmaExtras.Menu {
        id: fileMenu
        property point openAt
        readonly property bool isDir: root.menuEntry !== null && root.menuEntry.isDir

        onStatusChanged: {
            if (status === PlasmaExtras.Menu.Closed)
                Qt.callLater(() => {
                    root.menuOpen = false;
                    // Give the stack its keyboard back (for renaming, Esc, ...)
                    if (root.popupOpen)
                        root.fan.requestActivate();
                });
        }

        PlasmaExtras.MenuItem {
            section: true
            visible: root.menuMany
            text: i18np("%1 item", "%1 items", root.menuEntries.length)
        }
        PlasmaExtras.MenuItem {
            text: root.menuMany ? i18n("&Open All") : i18n("&Open")
            icon: fileMenu.isDir && !root.menuMany ? "folder-open" : "document-open"
            onClicked: {
                if (root.menuMany) {
                    // Several folders can't all open in the stack, so they go to the file manager
                    root.menuEntries.forEach(e => Qt.openUrlExternally(e.url));
                    root.closeFan();
                } else {
                    root.openEntry(root.menuEntry, root.menuInGrid);
                }
            }
        }
        PlasmaExtras.MenuItem {
            visible: fileMenu.isDir && !root.menuMany
            text: i18n("Open in %1", root.fileManagerLabel)
            icon: "system-file-manager"
            onClicked: root.openUrl(root.menuEntry.url)
        }
        PlasmaExtras.MenuItem {
            id: openWithSection
            section: true
            text: i18n("Open With")
            visible: root.openWithApps.length > 0
        }
        // (the apps go here, see the Instantiator below)
        PlasmaExtras.MenuItem {
            id: otherAppItem
            // KDE's chooser only takes one file at a time
            visible: !root.menuMany
            text: i18n("Other Application…")
            icon: "application-x-executable"
            onClicked: {
                root.fileAction(`choose ${root.shellQuote(root.menuEntry.url)}`);
                root.closeFan();
            }
        }
        PlasmaExtras.MenuItem {
            separator: true
        }
        PlasmaExtras.MenuItem {
            text: i18n("Show in %1", root.fileManagerLabel)
            icon: "document-open-folder"
            onClicked: {
                root.fileAction(`show ${root.quotedUrls(root.menuEntries)}`);
                root.closeFan();
            }
        }
        PlasmaExtras.MenuItem {
            visible: !root.menuMany && !root.recentMode   // renamed, it would just drop off Recent Files
            text: i18n("&Rename…")
            icon: "edit-rename"
            onClicked: root.startRename(root.menuEntry)
        }
        PlasmaExtras.MenuItem {
            text: root.menuMany ? i18n("&Copy Locations") : i18n("&Copy Location")
            icon: "edit-copy-path"
            onClicked: root.fileAction(`copy ${root.shellQuote(root.menuEntries.map(e => e.path).join("\n"))}`)
        }
        PlasmaExtras.MenuItem {
            separator: true
        }
        PlasmaExtras.MenuItem {
            text: i18n("Move to &Bin")
            icon: "user-trash"
            onClicked: {
                root.fileAction(`trash ${root.quotedUrls(root.menuEntries)}`);
                root.clearSelection();
            }
        }
    }

    Instantiator {
        model: root.openWithApps
        delegate: PlasmaExtras.MenuItem {
            text: modelData.name
            icon: modelData.icon || "application-x-executable"
            onClicked: {
                root.fileAction(`launch ${root.shellQuote(modelData.id)} ${root.quotedUrls(root.menuEntries)}`);
                root.closeFan();
            }
        }
        onObjectAdded: (index, object) => fileMenu.addMenuItem(object, otherAppItem)
        onObjectRemoved: (index, object) => fileMenu.removeMenuItem(object)
    }

    // Name of the default file manager for the "Open in …" buttons (Dolphin, COSMIC Files, …).
    // Looked up again each time the stack opens, in case the default has changed.
    property string fileManagerName: ""
    property string fileManagerId: ""   // its desktop file, e.g. org.kde.dolphin.desktop
    readonly property string fileManagerLabel: fileManagerName || i18n("File Manager")

    function lookUpFileManager() {
        runner.connectSource(`bash ${shellQuote(scriptPath("file-manager-name.sh"))} # file-manager ${Date.now()}`);
    }

    Component.onCompleted: {
        lookUpFileManager();
        refreshRecent();
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: (source, data) => {
            if (source.indexOf("# recent ") >= 0) {
                root.recentFound(data.stdout);
            } else if (source.indexOf("# new ") >= 0) {
                root.newFound(data.stdout);
            } else if (source.indexOf("# apps ") >= 0) {
                root.appsFound(source.slice(source.indexOf("# apps ") + 7), data.stdout);
            } else if (source.indexOf("# rename ") >= 0) {
                root.renameDone(source.slice(source.indexOf("# rename ") + 9), data.stdout, data["exit code"]);
            } else if (source.indexOf("# file-manager ") >= 0) {
                const lines = data.stdout.split("\n");
                root.fileManagerName = lines[0].trim();
                root.fileManagerId = (lines[1] || "").trim();
            } else if (source.indexOf("# thumbs ") >= 0) {
                root.thumbsDone(source.slice(source.indexOf("# thumbs ") + 9), data.stdout);
            } else if (source.indexOf("# download-sizes ") >= 0) {
                const sizes = {};
                for (const line of data.stdout.split("\n")) {
                    const space = line.indexOf(" ");
                    if (space > 0)
                        sizes[line.slice(space + 1)] = Number(line.slice(0, space));
                }
                root.downloadSizes = sizes;
            }
            disconnectSource(source);
        }
    }

    function openUrl(url) {
        closeFan();
        Qt.openUrlExternally(url);
    }

    // A file's picture: a preview for images, otherwise an icon for its type. No entry = the file manager.
    component FileIcon: Item {
        id: fileIcon
        property var entry: null
        // With "Show previews" off, every file just gets its icon
        readonly property bool previewsOn: Plasmoid.configuration.showPreviews
        readonly property string thumb: previewsOn && entry !== null && !entry.downloading ? (root.thumbs[entry.url.toString()] || "") : ""
        readonly property bool preview: previewsOn && entry !== null && !entry.downloading && (thumb !== "" || root.isImage(entry.suffix))

        // A made preview (PDF page, video frame, ...) gets a thin edge so white pages don't vanish
        Rectangle {
            visible: fileIcon.thumb !== "" && picture.status === Image.Ready
            anchors.centerIn: picture
            width: picture.paintedWidth + 2
            height: picture.paintedHeight + 2
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.25)
        }

        Image {
            id: picture
            anchors.fill: parent
            visible: fileIcon.preview
            source: visible ? (fileIcon.thumb || fileIcon.entry.url) : ""
            sourceSize: Qt.size(parent.width * 2, parent.height * 2)
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
        }

        Kirigami.Icon {
            anchors.fill: parent
            visible: !fileIcon.preview
            source: fileIcon.entry === null ? "system-file-manager" : root.iconFor(fileIcon.entry)
            fallback: "unknown"
            // Downloads still coming in are faded, with a pulsing arrow badge
            opacity: fileIcon.entry !== null && fileIcon.entry.downloading ? 0.45 : 1
        }

        // New since the stack was last looked at
        Rectangle {
            visible: fileIcon.entry !== null && root.newPaths.indexOf(fileIcon.entry.path) >= 0
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: -width * 0.15
            width: Math.round(parent.width * 0.26)
            height: width
            radius: width / 2
            color: Kirigami.Theme.highlightColor
            border.color: "white"
            border.width: Math.max(1, width * 0.12)
        }

        Rectangle {
            visible: fileIcon.entry !== null && fileIcon.entry.downloading
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: parent.width * 0.42
            height: width
            radius: width / 2
            color: Kirigami.Theme.highlightColor

            Kirigami.Icon {
                anchors.fill: parent
                anchors.margins: parent.width * 0.18
                source: "go-down"
                color: "white"
                isMask: true
            }

            SequentialAnimation on opacity {
                running: parent.visible
                loops: Animation.Infinite
                NumberAnimation { to: 0.5; duration: 700; easing.type: Easing.InOutSine }
                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
            }
        }
    }

    // Box for typing a file's new name, shown in place of its label while it's being renamed.
    // Enter or clicking elsewhere renames it, Esc gives up.
    component RenameField: QQC2.TextField {
        id: field
        property bool active: false
        property bool hadFocus: false
        visible: active
        horizontalAlignment: TextInput.AlignHCenter
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        onTextChanged: if (active) root.renameText = text
        onActiveChanged: {
            if (!active)
                return;
            hadFocus = false;
            text = root.renameText;
            // Like Dolphin: the name is picked out without its ending, so typing keeps ".pdf"
            const dot = text.lastIndexOf(".");
            const isDir = root.renamingPath !== "" && root.gridEntries.concat(root.allEntries)
                .some(e => e.path === root.renamingPath && e.isDir);
            select(0, dot > 0 && !isDir ? dot : text.length);
            forceActiveFocus();
        }
        onActiveFocusChanged: {
            if (activeFocus)
                hadFocus = true;
            else if (active && hadFocus)
                root.finishRename();
        }
        Keys.onReturnPressed: root.finishRename()
        Keys.onEnterPressed: root.finishRename()
        Keys.onEscapePressed: event => {
            root.cancelRename();
            event.accepted = true;   // don't close the stack as well
        }
        QQC2.ToolTip.visible: active && root.renameError !== ""
        QQC2.ToolTip.text: root.renameError
    }

    // Lets a file be dragged out to the file manager, the desktop or any app that takes files.
    // Put one inside a MouseArea and point the MouseArea's drag.target at it.
    component FileDrag: Item {
        id: dragItem
        property var entry: null
        property var grab: null    // the picture must stay referenced for its url to stay valid
        property bool active: false
        visible: false
        Drag.active: active && entry !== null && !entry.downloading
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
        // The whole selection if this file is part of it
        Drag.mimeData: entry ? { "text/uri-list": root.targetsFor(entry).map(e => e.url.toString()).join("\r\n") } : {}
        Drag.onDragStarted: root.dragging = true
        Drag.onDragFinished: {
            root.dragging = false;
            // (unless it was dropped on a folder in the stack and the Move / Copy menu is up)
            Qt.callLater(() => {
                if (!root.menuOpen)
                    root.closeFan();
            });
        }

        // Picture under the cursor while dragging
        function prepare(iconItem) {
            iconItem.grabToImage(result => {
                dragItem.grab = result;
                dragItem.Drag.imageSource = result.url;
            });
        }
    }

    NumberAnimation {
        id: openAnim
        target: root
        property: "progress"
        from: 0; to: 1
        duration: root.openDuration
        easing.type: Easing.InOutCubic
    }
    NumberAnimation {
        id: closeAnim
        target: root
        property: "progress"
        to: 0
        duration: root.closeDuration
        easing.type: Easing.InOutCubic
        onFinished: if (root.closing && root.fan) root.fan.visible = false
    }

    fullRepresentation: MouseArea {
        id: compact
        objectName: "folderStackButton"
        property real hop: 0   // how far the icon has bounced, as a share of its size
        hoverEnabled: true
        // Square, the size of the other dock icons
        Layout.minimumWidth: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? 0 : height
        Layout.minimumHeight: Plasmoid.formFactor === PlasmaCore.Types.Vertical ? width : 0
        Component.onCompleted: {
            root.dockIcon = compact;
            dropMenu.visualParent = compact;
        }
        onClicked: {
            folderTip.hideImmediately();
            // A click on the dock icon while the fan is open first closes it via deactivation; don't reopen straight away
            if (root.popupOpen || Date.now() - root.lastClosed < 250)
                root.closeFan();
            else
                root.openFan();
        }

        Kirigami.Icon {
            anchors.fill: parent
            source: Plasmoid.configuration.icon || root.folderIcon()
            active: compact.containsMouse || dropZone.hovering
            // Swells a little while files are held over it, like the macOS dock
            scale: dropZone.hovering ? 1.15 : 1
            Behavior on scale { NumberAnimation { duration: 120 } }
            // Hops away from the screen edge the dock is on
            transform: Translate {
                x: root.dockOnLeft ? compact.hop * compact.width : root.dockOnRight ? -compact.hop * compact.width : 0
                y: root.fanAllowed ? (root.dockOnTop ? 1 : -1) * compact.hop * compact.height : 0
            }
        }

        // Hops twice when a download finishes, like the macOS Downloads stack
        // (kept low: a widget can't draw above the top edge of the dock)
        SequentialAnimation {
            id: bounce
            NumberAnimation { target: compact; property: "hop"; to: 0.25; duration: 180; easing.type: Easing.OutQuad }
            NumberAnimation { target: compact; property: "hop"; to: 0; duration: 180; easing.type: Easing.InQuad }
            NumberAnimation { target: compact; property: "hop"; to: 0.12; duration: 140; easing.type: Easing.OutQuad }
            NumberAnimation { target: compact; property: "hop"; to: 0; duration: 140; easing.type: Easing.InQuad }
        }

        Connections {
            target: root
            function onDownloadFinished() { bounce.restart() }
        }

        // Something is downloading into this folder: a bar across the icon with a segment sliding back and forth
        // (browsers don't say how big a download will be, so it can't show a percentage)
        Rectangle {
            id: downloadBar
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * 0.1
            width: parent.width * 0.72
            height: Math.max(4, Math.round(parent.height * 0.11))
            radius: height / 2
            color: Qt.rgba(0, 0, 0, 0.6)
            border.color: Qt.rgba(1, 1, 1, 0.5)
            visible: root.activeDownloads.length > 0
            clip: true

            Rectangle {
                width: parent.width * 0.4
                height: parent.height
                radius: parent.radius
                color: Kirigami.Theme.highlightColor

                SequentialAnimation on x {
                    running: downloadBar.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: downloadBar.width * 0.6; duration: 800; easing.type: Easing.InOutSine }
                    NumberAnimation { from: downloadBar.width * 0.6; to: 0; duration: 800; easing.type: Easing.InOutSine }
                }
            }
        }

        // How many new files, in the corner like the counts on the dock's app icons
        Rectangle {
            readonly property int count: root.newPaths.length
            visible: count > 0 && !root.popupOpen
            anchors.right: parent.right
            anchors.top: parent.top
            height: Math.round(parent.height * 0.42)
            width: Math.max(height, countText.implicitWidth + height * 0.5)
            radius: height / 2
            color: Kirigami.Theme.highlightColor
            border.color: Qt.rgba(1, 1, 1, 0.8)

            Text {
                id: countText
                anchors.centerIn: parent
                color: "white"
                font.bold: true
                font.pixelSize: parent.height * 0.62
                text: parent.count > 99 ? "99+" : parent.count
            }
        }

        // Folder name on hover, the same style of tooltip as the app icons on the dock
        PlasmaCore.ToolTipArea {
            id: folderTip
            anchors.fill: parent
            mainText: root.folderName
            location: Plasmoid.location
            active: !root.popupOpen && !root.dragging
        }

        // KDE's own drop area rather than Qt's: it tells the panel to drop its
        // "new widget goes here" gap when we take the files
        DragDrop.DropArea {
            id: dropZone
            property bool hovering: false
            anchors.fill: parent
            enabled: !root.recentMode   // Recent Files isn't a place files can go
            preventStealing: true
            onDragEnter: event => {
                hovering = event.mimeData.hasUrls;
                root.closePanelGap();
                root.dragOverDock = hovering;
                // Held here for a moment: open the stack so the files can go into a folder inside it
                if (hovering && !root.popupOpen)
                    springOpen.restart();
            }
            onDragLeave: {
                hovering = false;
                root.dragOverDock = false;
                springOpen.stop();
            }
            onDrop: event => {
                hovering = false;
                root.dragOverDock = false;
                springOpen.stop();
                root.closePanelGap();
                if (!event.mimeData.hasUrls) {
                    event.ignore();
                    return;
                }
                root.handleDrop(event.mimeData.urls, event.modifiers, root.folderPath());
                if (root.springLoaded)
                    root.closeFan();
                // Report a copy whatever we do, so the app the files came from never deletes its own copy
                event.accept(Qt.CopyAction);
            }
        }
    }

    // The popup (fan or grid) is only built when the stack opens and is thrown away when it closes,
    // so a closed stack costs next to nothing
    property Item dockIcon: null
    readonly property var fan: fanLoader.item
    readonly property bool popupOpen: fan !== null && fan.visible

    Loader {
        id: fanLoader
        active: false
        sourceComponent: PlasmaCore.Dialog {
            objectName: "folderStackWindow"
            property bool sequoiaNoBlur: !root.showGrid
            property bool hadFocus: false
            location: Plasmoid.location
            type: PlasmaCore.Dialog.PopupMenu
            backgroundHints: PlasmaCore.Dialog.NoBackground
            flags: Qt.WindowStaysOnTopHint
            // Keep the surface alive until the files finish folding.
            hideOnWindowDeactivate: false
            onActiveChanged: {
                if (active) hadFocus = true;
                else if (visible && hadFocus && !root.dragging && !root.menuOpen && !root.springLoaded)
                    root.closeFan();
            }
            onVisibleChanged: {
                if (!visible) {
                    root.lastClosed = Date.now();
                    // Not straight away: the window is still in the middle of hiding
                    openAnim.stop();
                    closeAnim.stop();
                    Qt.callLater(() => { if (root.fan && !root.fan.visible) fanLoader.active = false; });
                }
            }

            mainItem: Item {
                id: popupBody
                enabled: !root.closing
                width: root.showGrid ? gridBox.width : canvas.width
                height: root.showGrid ? gridBox.height + gridStack.depth * gridStack.step : canvas.height
                focus: true
                // Esc drops the selection first, then closes the stack
                Keys.onEscapePressed: root.selection.length > 0 ? root.clearSelection() : root.closeFan()
                Keys.onPressed: event => {
                    // Typing a letter starts filtering; the fan shows only a few files, so it turns into the grid
                    const typed = event.text.length === 1 && event.text >= " " && event.text !== "\x7f"
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier));
                    if (typed && root.renamingPath === "") {
                        root.showGrid = true;
                        searchField.text = event.text;
                        searchField.forceActiveFocus();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Backspace && root.browsing) {
                        root.goBack();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                        root.selectAll();
                        event.accepted = true;
                    }
                }

                // Files dragged over the open stack: held over a folder it opens, dropped they go in
                // (whatever folder is under the pointer, else the one on show)
                DragDrop.DropArea {
                    anchors.fill: parent
                    enabled: !root.recentMode
                    onDragEnter: event => {
                        root.dragOverPopup = event.mimeData.hasUrls;
                        root.dragMovedOver(popupBody, event.x, event.y);
                    }
                    onDragMove: event => root.dragMovedOver(popupBody, event.x, event.y)
                    onDragLeave: {
                        root.dragOverPopup = false;
                        root.dropHover = null;
                        springFolder.stop();
                    }
                    onDrop: event => {
                        if (!event.mimeData.hasUrls) {
                            event.ignore();
                            return;
                        }
                        root.droppedOnStack(popupBody, event);
                        // A copy as far as the other app knows, so it never deletes its own files
                        event.accept(Qt.CopyAction);
                    }
                }

                // Fan: files climb a gentle curve above the dock icon. With "Stacked on top", a folder clicked
                // in the fan gets its own fan; the earlier ones slide right, dim, and go back to their folder when clicked.
                Item {
                    id: canvas
                    visible: !root.showGrid
                    readonly property int depth: root.fanLevels.length - 1
                    readonly property real shift: root.iconSize + Kirigami.Units.largeSpacing * 2
                    // Tallest fan, plus one row at the top for "Open in <file manager>"
                    readonly property int rows: Math.min(root.maxItems, Math.max(...root.fanLevels.map(list => list.length))) + 1
                    readonly property real topAngle: (rows - 0.5) * root.rowHeight / root.radius
                    readonly property real leftExtent: root.iconSize / 2 + Kirigami.Units.smallSpacing + root.labelWidth
                    readonly property real rightExtent: root.radius * (1 - Math.cos(topAngle)) + root.iconSize + depth * shift
                    width: 2 * Math.max(leftExtent, rightExtent) + Kirigami.Units.gridUnit * 2
                    // Tilted, the top row's label rises above its icon; room for that too
                    readonly property real tiltRise: Plasmoid.configuration.straightLabels ? 0 : (root.labelWidth + root.iconSize) * Math.sin(topAngle)
                    height: root.radius * Math.sin(topAngle) + root.rowHeight + Kirigami.Units.gridUnit + tiltRise

                    Repeater {
                        model: canvas.visible ? root.fanLevels.length : 0

                        delegate: Item {
                            id: fanLayer
                            readonly property int level: index
                            readonly property bool current: level === canvas.depth
                            readonly property var list: root.fanLevels[level] || []
                            readonly property var shown: list.slice(0, root.maxItems)
                            readonly property int hidden: Math.max(0, list.length - root.maxItems)
                            // (While going back, a fan about to be removed can briefly outlive its entry in browsePath)
                            readonly property url folder: level > 0 && root.browsePath[level - 1] ? root.browsePath[level - 1].url : root.folderUrl
                            // A newly opened fan grows out of the dock like the first one
                            readonly property real grow: current && level > 0 ? root.pushProgress : 1

                            width: canvas.width
                            height: canvas.height
                            z: level
                            x: (canvas.depth - level) * canvas.shift
                            Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                            opacity: current ? 1 : 0.5

                            Repeater {
                                model: fanLayer.shown.length + 1

                                delegate: Item {
                                    id: row
                                    objectName: "folderStackRow" + fanLayer.level + "_" + index
                                    readonly property real unfolding: root.rowProgress(index, fanLayer.shown.length, fanLayer.grow)
                                    readonly property bool isMore: index === fanLayer.shown.length
                                    visible: !isMore || root.canOpenStackFolder || fanLayer.level > 0
                                    readonly property var entry: isMore ? null : fanLayer.shown[index]
                                    // Bottom row (newest) sits just above the dock, later rows climb the arc and lean
                                    readonly property real angle: (index + 0.5) * root.rowHeight / root.radius
                                    readonly property real cx: canvas.width / 2 + root.radius * (1 - Math.cos(angle))
                                    readonly property real cy: canvas.height - root.radius * Math.sin(angle) - root.rowHeight / 2
                                    readonly property bool renaming: entry !== null && fanLayer.current && root.renamingPath === entry.path
                                    readonly property bool selected: entry !== null && fanLayer.current && root.selection.indexOf(entry.path) >= 0
                                    // For files dragged over the stack: an earlier fan means its folder, a folder means itself
                                    readonly property var dropTarget: !fanLayer.current ? { level: fanLayer.level }
                                        : entry !== null && entry.isDir && !entry.downloading ? { entry: entry } : null
                                    readonly property bool dropHere: entry !== null && root.dropHover !== null && root.dropHover.entry !== undefined
                                        && root.dropHover.entry.path === entry.path

                                    width: root.labelWidth + Kirigami.Units.smallSpacing + root.iconSize
                                    height: root.rowHeight
                                    x: canvas.width / 2 - width + root.iconSize / 2 + (cx - canvas.width / 2) * unfolding
                                    // (mirrored top to bottom when the dock is at the top of the screen)
                                    readonly property real foldY: root.dockOnTop ? -height / 2 : canvas.height - height / 2
                                    readonly property real openY: root.dockOnTop ? canvas.height - cy - height / 2 : cy - height / 2
                                    y: foldY * (1 - unfolding) + openY * unfolding
                                    opacity: Math.min(1, unfolding * 5)
                                    transform: [Scale {
                                        origin.x: row.width - root.iconSize / 2
                                        origin.y: row.height / 2
                                        xScale: 0.18 + 0.82 * row.unfolding
                                        yScale: xScale
                                    }, Rotation {
                                        origin.x: row.width - root.iconSize / 2
                                        origin.y: row.height / 2
                                        // "Straight" keeps every row level (still placed along the curve): tilted text is never quite as sharp
                                        angle: Plasmoid.configuration.straightLabels ? 0 : (root.dockOnTop ? -1 : 1) * (-18 * (1 - row.unfolding) + row.angle * 180 / Math.PI * row.unfolding)
                                    }]

                                    MouseArea {
                                        id: hit
                                        // Only the icon and its label are clickable, not the empty space beside the label.
                                        // Fans behind the current one show just their icons.
                                        x: fanLayer.current ? label.x : parent.width - root.iconSize
                                        width: parent.width - x
                                        height: parent.height
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                                        drag.target: fanLayer.current && !row.isMore && !(pressedButtons & Qt.RightButton) ? rowDrag : null
                                        onPressed: mouse => {
                                            if (fanLayer.current && !row.isMore && mouse.button === Qt.LeftButton)
                                                rowDrag.prepare(icon);
                                        }
                                        onClicked: mouse => {
                                            if (root.renamingPath !== "") {
                                                // A click anywhere else finishes the rename first, like Dolphin
                                                if (!row.renaming)
                                                    root.finishRename();
                                            } else if (mouse.button === Qt.RightButton) {
                                                if (fanLayer.current && !row.isMore)
                                                    root.showFileMenu(row.entry, hit, mouse.x, mouse.y, false);
                                            } else if (fanLayer.current && root.selectClick(row.entry, mouse.modifiers)) {
                                                return;   // Ctrl/Shift-click picked it
                                            } else if (!fanLayer.current)
                                                root.backTo(fanLayer.level);
                                            else if (row.isMore)
                                                root.openStackFolder(fanLayer.folder);
                                            else if (row.entry.downloading)
                                                return;   // nothing to open until it's finished
                                            else {
                                                root.clearSelection();
                                                root.openEntry(row.entry, false);
                                            }
                                        }

                                        FileDrag {
                                            id: rowDrag
                                            entry: row.entry
                                            active: hit.drag.active
                                        }
                                    }

                                    Rectangle {
                                        id: label
                                        opacity: Math.max(0, (row.unfolding - 0.3) / 0.7)
                                        visible: fanLayer.current
                                        anchors.right: icon.left
                                        anchors.rightMargin: Kirigami.Units.smallSpacing
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: row.renaming ? root.labelWidth : Math.min(root.labelWidth, text.implicitWidth + Kirigami.Units.largeSpacing * 2)
                                        height: row.renaming ? fanRename.implicitHeight : text.implicitHeight + Kirigami.Units.smallSpacing * 2
                                        radius: height / 2
                                        color: hit.containsMouse || row.selected || row.dropHere ? Kirigami.Theme.highlightColor : Qt.rgba(0.1, 0.1, 0.1, 0.78)

                                        RenameField {
                                            id: fanRename
                                            anchors.fill: parent
                                            active: row.renaming
                                        }

                                        Text {
                                            id: text
                                            visible: !row.renaming
                                            anchors.centerIn: parent
                                            width: parent.width - Kirigami.Units.largeSpacing * 2
                                            horizontalAlignment: Text.AlignHCenter
                                            elide: Text.ElideMiddle
                                            color: "white"
                                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.95
                                            text: row.isMore
                                                ? (fanLayer.hidden > 0 ? i18n("%1 more in %2", fanLayer.hidden, root.fileManagerLabel) : i18n("Open in %1", root.fileManagerLabel))
                                                : row.entry.downloading
                                                    ? i18n("%1 · downloading %2", row.entry.name, root.sizeText(row.entry))
                                                    : row.entry.name
                                        }
                                    }

                                    // Selected: a highlight behind the icon as well as the label
                                    Rectangle {
                                        visible: row.selected
                                        anchors.centerIn: icon
                                        width: root.iconSize + Kirigami.Units.smallSpacing * 2
                                        height: width
                                        radius: Kirigami.Units.cornerRadius * 2
                                        color: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.45)
                                    }

                                    FileIcon {
                                        id: icon
                                        entry: row.entry
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: root.iconSize
                                        height: root.iconSize
                                        scale: hit.containsMouse ? 1.08 : 1
                                        Behavior on scale { NumberAnimation { duration: 90 } }
                                    }
                                }
                            }
                        }
                    }
                }

                // Stacked folders: the folders you came through peek out above the open one, furthest back on top.
                // Clicking one closes everything in front of it.
                Repeater {
                    id: gridStack
                    readonly property int depth: root.showGrid && root.stackedFolders ? root.browsePath.length : 0
                    readonly property real step: header.height + gridBox.pad * 1.5
                    model: depth

                    delegate: Rectangle {
                        // index 0 = the stack's own folder
                        readonly property int behind: gridStack.depth - index   // how many folders sit in front of it
                        readonly property var dropTarget: ({ level: index })
                        readonly property bool dropHere: root.dropHover !== null && root.dropHover.level === index
                        Kirigami.Theme.colorSet: Kirigami.Theme.Window
                        Kirigami.Theme.inherit: false
                        z: 0
                        width: gridBox.width
                        height: gridBox.height
                        // Peeking out on the side away from the dock: above it, or below it for a top dock
                        y: root.dockOnTop ? gridBox.y + behind * gridStack.step : gridBox.y - behind * gridStack.step
                        // Slightly smaller and darker the further back it is
                        scale: 1 - 0.03 * behind
                        transformOrigin: root.dockOnTop ? Item.Bottom : Item.Top
                        radius: gridBox.radius
                        color: Qt.darker(Kirigami.Theme.backgroundColor, 1 + 0.12 * behind)
                        border.color: gridBox.border.color
                        opacity: root.progress

                        Kirigami.Heading {
                            // On the part that shows
                            anchors.top: root.dockOnTop ? undefined : parent.top
                            anchors.bottom: root.dockOnTop ? parent.bottom : undefined
                            anchors.topMargin: gridBox.pad
                            anchors.bottomMargin: gridBox.pad
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - gridBox.pad * 2
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            level: 4
                            opacity: strip.containsMouse || parent.dropHere ? 1 : 0.65
                            text: index > 0 && root.browsePath[index - 1] ? root.browsePath[index - 1].name : root.folderName
                        }

                        MouseArea {
                            id: strip
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.backTo(index)
                        }
                    }
                }

                // Grid: every file in a rounded box, scrolling when there are lots
                Rectangle {
                    id: gridBox
                    readonly property bool dropOpaque: true
                    visible: root.showGrid
                    z: 1    // above the stacked folders behind it
                    y: root.dockOnTop ? 0 : gridStack.depth * gridStack.step
                    Kirigami.Theme.colorSet: Kirigami.Theme.Window
                    Kirigami.Theme.inherit: false

                    readonly property int pad: Kirigami.Units.largeSpacing
                    readonly property int cellWidth: Kirigami.Units.gridUnit * 6
                    readonly property int cellHeight: root.iconSize + Kirigami.Units.gridUnit * 3
                    // Never smaller than the top-level view, so the box doesn't jump about while browsing subfolders
                    readonly property int columns: Math.max(3, Math.min(5, Math.max(root.allEntries.length, root.gridEntries.length)))
                    readonly property int visibleRows: Math.max(1, Math.min(4, Math.ceil(Math.max(root.allEntries.length, root.gridEntries.length) / columns)))

                    width: columns * cellWidth + pad * 2
                    height: header.height + visibleRows * cellHeight + footer.height + pad * 4
                    radius: Kirigami.Units.cornerRadius * 3
                    color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.96)
                    border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)
                    // Grows out of the dock icon, from whichever edge the dock is on
                    opacity: root.progress * (root.stackedFolders ? root.pushProgress : 1)
                    scale: 0.85 + 0.15 * root.progress
                    transformOrigin: root.dockOnTop ? Item.Top : root.dockOnLeft ? Item.Left : root.dockOnRight ? Item.Right : Item.Bottom
                    // A newly stacked folder slides into place from the dock's side
                    transform: Translate { y: root.stackedFolders ? (root.dockOnTop ? -1 : 1) * (1 - root.pushProgress) * gridStack.step : 0 }

                    // Folder name, with a back button once you've gone into a subfolder (unless they're stacked)
                    Item {
                        id: header
                        anchors.top: parent.top
                        anchors.topMargin: gridBox.pad
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: gridBox.pad
                        anchors.rightMargin: gridBox.pad
                        height: Math.max(title.implicitHeight, backButton.implicitHeight)

                        QQC2.ToolButton {
                            id: backButton
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(implicitWidth, parent.width / 3)
                            visible: root.browsing && !root.stackedFolders
                            icon.name: "go-previous"
                            text: root.browsePath.length > 1 ? root.browsePath[root.browsePath.length - 2].name : root.folderName
                            onClicked: root.goBack()
                        }

                        // While filtering, the box for the text sits where the folder name was
                        Kirigami.SearchField {
                            id: searchField
                            anchors.centerIn: parent
                            width: title.width
                            visible: root.filterText !== ""
                            placeholderText: i18n("Filter")
                            autoAccept: false
                            onTextChanged: root.filterText = text
                            // Emptied (by Backspace or its clear button): back to the folder name, keys back to the stack
                            onVisibleChanged: if (!visible) popupBody.forceActiveFocus()
                            Keys.onEscapePressed: event => {
                                text = "";
                                event.accepted = true;   // don't close the stack as well
                            }
                            // Enter opens the first match. (Not onAccepted: the field also sends that
                            // after a pause in typing and when its clear button is clicked.)
                            function openFirst() {
                                const first = root.shownEntries.find(e => !e.downloading);
                                if (first && text.trim() !== "")
                                    root.openEntry(first, true);
                            }
                            Keys.onReturnPressed: openFirst()
                            Keys.onEnterPressed: openFirst()

                            Connections {
                                target: root
                                function onFilterTextChanged() {
                                    if (searchField.text !== root.filterText)
                                        searchField.text = root.filterText;
                                }
                            }
                        }

                        Kirigami.Heading {
                            id: title
                            visible: !searchField.visible
                            anchors.centerIn: parent
                            width: parent.width - (backButton.visible ? backButton.width * 2 + Kirigami.Units.smallSpacing * 2 : 0)
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            level: 4
                            text: root.gridFolderName
                        }
                    }

                    GridView {
                        id: grid
                        anchors.top: header.bottom
                        anchors.topMargin: gridBox.pad
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: gridBox.columns * gridBox.cellWidth
                        height: gridBox.visibleRows * gridBox.cellHeight
                        cellWidth: gridBox.cellWidth
                        cellHeight: gridBox.cellHeight
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: gridBox.visible ? root.shownEntries : []
                        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

                        delegate: MouseArea {
                            id: cell
                            width: gridBox.cellWidth
                            height: gridBox.cellHeight
                            readonly property bool renaming: root.renamingPath === modelData.path
                            readonly property bool selected: root.selection.indexOf(modelData.path) >= 0
                            // Folders take files dragged over the stack
                            readonly property var dropTarget: modelData.isDir && !modelData.downloading ? { entry: modelData } : null
                            readonly property bool dropHere: root.dropHover !== null && root.dropHover.entry !== undefined
                                && root.dropHover.entry.path === modelData.path
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            drag.target: pressedButtons & Qt.RightButton ? null : cellDrag
                            onPressed: mouse => {
                                if (mouse.button === Qt.LeftButton)
                                    cellDrag.prepare(cellIcon);
                            }
                            // Folders open right here in the grid, like macOS
                            onClicked: mouse => {
                                if (root.renamingPath !== "") {
                                    // A click anywhere else finishes the rename first, like Dolphin
                                    if (!cell.renaming)
                                        root.finishRename();
                                } else if (mouse.button === Qt.RightButton) {
                                    root.showFileMenu(modelData, cell, mouse.x, mouse.y, true);
                                } else if (root.selectClick(modelData, mouse.modifiers)) {
                                    return;   // Ctrl/Shift-click picked it
                                } else if (!modelData.downloading) {   // nothing to open until it's finished
                                    root.clearSelection();
                                    root.openEntry(modelData, true);
                                }
                            }

                            FileDrag {
                                id: cellDrag
                                entry: modelData
                                active: cell.drag.active
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 2
                                radius: Kirigami.Units.cornerRadius
                                color: Kirigami.Theme.highlightColor
                                opacity: cell.selected || cell.dropHere ? 0.5 : cell.containsMouse ? 0.25 : 0
                            }

                            FileIcon {
                                id: cellIcon
                                entry: modelData
                                anchors.top: parent.top
                                anchors.topMargin: Kirigami.Units.smallSpacing
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: root.iconSize
                                height: root.iconSize
                            }

                            RenameField {
                                anchors.top: cellIcon.bottom
                                anchors.topMargin: Kirigami.Units.smallSpacing
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width - 2
                                active: cell.renaming
                            }

                            QQC2.Label {
                                visible: !cell.renaming
                                anchors.top: cellIcon.bottom
                                anchors.topMargin: Kirigami.Units.smallSpacing
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width - Kirigami.Units.smallSpacing * 2
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                maximumLineCount: modelData.downloading ? 3 : 2
                                elide: Text.ElideRight
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: modelData.downloading
                                    ? modelData.name + "\n" + i18n("Downloading… %1", root.sizeText(modelData))
                                    : modelData.name
                            }
                        }

                        QQC2.Label {
                            anchors.centerIn: parent
                            visible: root.shownEntries.length === 0 && (!root.browsing || browseModel.status === FolderListModel.Ready)
                            opacity: 0.6
                            text: root.filterText !== "" ? i18n("Nothing called “%1”", root.filterText.trim()) : (root.recentMode && !root.browsing ? i18n("No recent files") : i18n("This folder is empty"))
                        }
                    }

                    QQC2.Button {
                        id: footer
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: gridBox.pad
                        anchors.horizontalCenter: parent.horizontalCenter
                        icon.name: "system-file-manager"
                        text: i18n("Open in %1", root.fileManagerLabel)
                        visible: root.canOpenStackFolder || root.browsing
                        onClicked: root.openStackFolder(root.gridFolderUrl)
                    }
                }
            }
        }
    }
}
