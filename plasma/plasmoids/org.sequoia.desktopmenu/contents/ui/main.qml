// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQml.Models
import QtCore
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.plasma.plasma5support as Plasma5Support
import org.kde.taskmanager as TaskManager
import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg
import org.kde.kwindowsystem as KWindowSystem

PlasmoidItem {
    id: root
    objectName: "sequoiaDesktopMenu"
    preferredRepresentation: fullRepresentation
    implicitWidth: fullRepresentationItem ? fullRepresentationItem.implicitWidth : 260
    implicitHeight: 28
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    Plasmoid.status: desktopActive || Plasmoid.userConfiguring
        ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.HiddenStatus

    // An active app without exported menus must still suppress this bar.
    // No screen/desktop filters: an app on another monitor is still active.
    TaskManager.TasksModel {
        id: tasks
        groupMode: TaskManager.TasksModel.GroupDisabled
    }
    // Track the role notifications as well as row insertion/removal. Binding
    // to QModelIndex.valid alone misses validity changes in some Qt versions.
    property bool applicationActive: false
    function updateFocus() {
        let active = false;
        for (let i = 0; i < windows.count; ++i) {
            const window = windows.objectAt(i);
            if (window && window.activeWindow) { active = true; break; }
        }
        applicationActive = active;
    }
    Instantiator {
        id: windows
        model: tasks
        delegate: QtObject {
            required property var model
            readonly property bool activeWindow: model.IsActive
            onActiveWindowChanged: Qt.callLater(root.updateFocus)
        }
        onObjectAdded: Qt.callLater(root.updateFocus)
        onObjectRemoved: Qt.callLater(root.updateFocus)
    }
    readonly property bool desktopActive: KWindowSystem.KWindowSystem.showingDesktop || !applicationActive
    onDesktopActiveChanged: if (!desktopActive) popup.visible = false
    Layout.minimumWidth: desktopActive || Plasmoid.userConfiguring ? implicitWidth : 0
    Layout.preferredWidth: Layout.minimumWidth
    Layout.maximumWidth: Layout.minimumWidth
    Layout.fillHeight: true
    Plasmoid.onActivated: if (desktopActive) fullRepresentationItem.openMenu(0)

    function run(command) { executable.connectSource(command); }
    function shellQuote(value) { return "'" + value.replace(/'/g, "'\\''") + "'"; }
    function openFolder(location) {
        const path = StandardPaths.writableLocation(location);
        if (path) Qt.openUrlExternally("file://" + path.split("/").map(encodeURIComponent).join("/"));
    }
    function configureDesktop() {
        const script = "var d = desktopForScreen(" + Plasmoid.containment.screen + "); if (d) d.showConfigurationInterface();";
        run("qdbus-qt6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript " + shellQuote(script));
    }
    Plasma5Support.DataSource {
        id: executable
        engine: "executable"
        connectedSources: []
        onNewData: (sourceName, data) => {
            disconnectSource(sourceName);
            if (data["exit code"] !== 0) {
                console.warn("Sequoia Desktop Menu:", sourceName, data.stderr);
                root.message = i18n("The action could not be completed. %1", data.stderr || "");
                root.confirmRestart = false;
                messageDialog.visible = true;
            }
        }
    }
    readonly property var menus: [
        {title: i18n("Plasma"), items: [
            {text: i18n("About This System"), command: "kinfocenter"},
            {text: i18n("System Settings…"), command: "systemsettings"},
            {separator: true},
            {text: i18n("Restart Plasma Shell…"), action: "restart"}
        ]},
        {title: i18n("Desktop"), items: [
            {text: i18n("Configure Desktop and Wallpaper…"), action: "configure"},
            {text: i18n("Add Widgets…"), command: "qdbus-qt6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.toggleWidgetExplorer"},
            {text: i18n("Enter Edit Mode"), action: "edit"},
            {separator: true},
            {text: i18n("Display Settings…"), command: "systemsettings kcm_kscreen"}
        ]},
        {title: i18n("Go"), items: [
            {text: i18n("Home"), folder: StandardPaths.HomeLocation},
            {text: i18n("Desktop"), folder: StandardPaths.DesktopLocation},
            {text: i18n("Documents"), folder: StandardPaths.DocumentsLocation},
            {text: i18n("Downloads"), folder: StandardPaths.DownloadLocation},
            {separator: true},
            {text: i18n("Trash"), url: "trash:/"}
        ]},
        {title: i18n("Help"), items: [
            {text: i18n("Plasma Handbook"), url: "help:/plasma-desktop"},
            {text: i18n("Keyboard Shortcuts…"), command: "systemsettings kcm_keys"}
        ]}
    ]
    property int menuIndex: 0
    function activate(entry) {
        popup.visible = false;
        if (entry.command) run(entry.command);
        else if (entry.url) Qt.openUrlExternally(entry.url);
        else if (entry.folder !== undefined) openFolder(entry.folder);
        else if (entry.action === "configure") configureDesktop();
        else if (entry.action === "edit") Plasmoid.containment.corona.editMode = true;
        else if (entry.action === "restart") {
            confirmRestart = true;
            message = i18n("The desktop and panels will briefly disappear. Your application windows will stay open.");
            messageDialog.visible = true;
            messageDialog.requestActivate();
        }
    }
    fullRepresentation: Row {
        id: bar
        objectName: "desktopMenuBar"
        visible: root.desktopActive || Plasmoid.userConfiguring
        readonly property int count: root.menus.length
        function openMenu(index) {
            root.menuIndex = index;
            popup.visualParent = buttons.itemAt(index);
            popup.visible = true;
            popup.requestActivate();
            menuContents.currentIndex = 0;
            menuContents.forceActiveFocus();
        }
        Repeater {
            id: buttons
            model: root.menus
            delegate: PC3.AbstractButton {
                id: button
                required property var modelData
                required property int index
                objectName: "desktopMenuButton-" + index
                text: modelData.title
                font.bold: index === 0
                implicitWidth: contentItem.implicitWidth + 18
                height: bar.height
                hoverEnabled: true
                activeFocusOnTab: true
                readonly property bool selected: popup.visible && root.menuIndex === index
                onPressed: selected ? popup.visible = false : bar.openMenu(index)
                onHoveredChanged: if (hovered && popup.visible && root.menuIndex !== index) bar.openMenu(index)
                Keys.onLeftPressed: buttons.itemAt((index + bar.count - 1) % bar.count).forceActiveFocus()
                Keys.onRightPressed: buttons.itemAt((index + 1) % bar.count).forceActiveFocus()
                Keys.onDownPressed: bar.openMenu(index)
                Keys.onReturnPressed: bar.openMenu(index)
                Keys.onSpacePressed: bar.openMenu(index)
                contentItem: PC3.Label {
                    text: button.text
                    font: button.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: button.selected || button.hovered || button.activeFocus
                        ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                }
                background: KSvg.FrameSvgItem {
                    imagePath: "widgets/menubaritem"
                    prefix: button.selected ? "pressed" : button.hovered || button.activeFocus ? "hover" : "normal"
                }
            }
        }
    }
    PlasmaCore.Dialog {
        id: popup
        objectName: "desktopMenuPopup"
        visible: false
        location: Plasmoid.location
        type: PlasmaCore.Dialog.PopupMenu
        hideOnWindowDeactivate: true
        mainItem: Column {
            id: menuContents
            width: Math.max(220, ...root.menus[root.menuIndex].items.map(entry => metrics.advanceWidth(entry.text || "") + 36))
            height: implicitHeight
            spacing: 0
            property int currentIndex: 0
            FontMetrics { id: metrics; font: Kirigami.Theme.defaultFont }
            function move(direction) {
                const entries = root.menus[root.menuIndex].items;
                do { currentIndex = (currentIndex + direction + entries.length) % entries.length; }
                while (entries[currentIndex].separator);
            }
            Keys.onUpPressed: move(-1)
            Keys.onDownPressed: move(1)
            Keys.onEscapePressed: popup.visible = false
            Keys.onReturnPressed: root.activate(root.menus[root.menuIndex].items[currentIndex])
            Keys.onSpacePressed: root.activate(root.menus[root.menuIndex].items[currentIndex])
            Keys.onLeftPressed: root.fullRepresentationItem.openMenu((root.menuIndex + root.menus.length - 1) % root.menus.length)
            Keys.onRightPressed: root.fullRepresentationItem.openMenu((root.menuIndex + 1) % root.menus.length)
            Repeater {
                model: root.menus[root.menuIndex].items
                delegate: Loader {
                    id: entryLoader
                    required property var modelData
                    required property int index
                    width: menuContents.width
                    sourceComponent: modelData.separator ? separator : menuEntry
                    Component { id: separator; PC3.MenuSeparator {} }
                    Component {
                        id: menuEntry
                        PC3.ItemDelegate {
                            text: entryLoader.modelData.text
                            highlighted: menuContents.currentIndex === entryLoader.index
                            Accessible.role: Accessible.MenuItem
                            onHoveredChanged: if (hovered) menuContents.currentIndex = entryLoader.index
                            onClicked: root.activate(entryLoader.modelData)
                        }
                    }
                }
            }
        }
    }
    property bool confirmRestart: false
    property string message: ""
    PlasmaCore.Dialog {
        id: messageDialog
        objectName: "desktopMenuMessage"
        visible: false
        visualParent: root.fullRepresentationItem
        location: Plasmoid.location
        type: PlasmaCore.Dialog.DialogWindow
        flags: Qt.Dialog
        hideOnWindowDeactivate: false
        onActiveChanged: if (active) cancelButton.forceActiveFocus()
        mainItem: ColumnLayout {
            width: 370
            PC3.Label {
                Layout.fillWidth: true
                text: root.confirmRestart ? i18n("Restart Plasma Shell?") : i18n("Desktop Menu")
                font.bold: true
            }
            PC3.Label { Layout.fillWidth: true; text: root.message; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                PC3.Button {
                    id: cancelButton
                    text: root.confirmRestart ? i18n("Cancel") : i18n("Close")
                    onClicked: messageDialog.visible = false
                }
                PC3.Button {
                    visible: root.confirmRestart
                    text: i18n("Restart Plasma Shell")
                    onClicked: {
                        messageDialog.visible = false;
                        root.run("systemctl --user --no-block restart plasma-plasmashell.service");
                    }
                }
            }
            Keys.onEscapePressed: messageDialog.visible = false
        }
    }
}
