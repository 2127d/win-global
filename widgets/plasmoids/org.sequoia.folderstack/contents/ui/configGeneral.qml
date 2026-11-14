import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.iconthemes as KIconThemes
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore

KCM.SimpleKCM {
    id: page
    property alias cfg_folder: folderField.text
    property alias cfg_sortMode: sortBox.currentIndex
    property alias cfg_viewMode: viewBox.currentIndex
    property alias cfg_subfolderMode: subfolderBox.currentIndex
    property alias cfg_maxItems: maxSpin.value
    property alias cfg_bounceOnDownload: bounceBox.checked
    property alias cfg_showPreviews: previewsBox.checked
    property alias cfg_markNew: newBox.checked
    property bool cfg_recentFiles
    property bool cfg_straightLabels
    property string cfg_icon
    // Kept by the widget itself, not shown here
    property string cfg_lastSeen

    // Plasma also hands every settings page each option's default value; we don't use them
    property var cfg_folderDefault
    property var cfg_viewModeDefault
    property var cfg_subfolderModeDefault
    property var cfg_sortModeDefault
    property var cfg_maxItemsDefault
    property var cfg_showPreviewsDefault
    property var cfg_bounceOnDownloadDefault
    property var cfg_markNewDefault
    property var cfg_recentFilesDefault
    property var cfg_straightLabelsDefault
    property var cfg_iconDefault
    property var cfg_lastSeenDefault

    Kirigami.FormLayout {
        QQC2.ComboBox {
            id: showBox
            Kirigami.FormData.label: i18n("Show:")
            model: [i18n("A folder"), i18n("Recently used files")]
            currentIndex: page.cfg_recentFiles ? 1 : 0
            onActivated: index => page.cfg_recentFiles = index === 1
        }

        QQC2.Label {
            visible: page.cfg_recentFiles
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            opacity: 0.7
            text: i18n("Files you've opened or saved lately, from any folder, newest first. This is the list KDE keeps; if it stays empty, check that System Settings → Recent Files is remembering files.")
        }

        RowLayout {
            enabled: !page.cfg_recentFiles
            Kirigami.FormData.label: i18n("Folder:")
            QQC2.TextField {
                id: folderField
                Layout.preferredWidth: Kirigami.Units.gridUnit * 16
                placeholderText: i18n("Downloads")
            }
            QQC2.Button {
                icon.name: "document-open-folder"
                text: i18n("Choose…")
                onClicked: picker.open()
            }
        }

        RowLayout {
            Kirigami.FormData.label: i18n("Icon:")

            QQC2.Button {
                id: iconButton
                Layout.preferredWidth: Kirigami.Units.iconSizes.large + Kirigami.Units.smallSpacing * 4
                Layout.preferredHeight: Layout.preferredWidth
                checkable: true
                checked: iconMenu.visible
                onClicked: iconMenu.popup(iconButton, 0, iconButton.height)

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.large
                    height: width
                    source: page.cfg_icon || page.automaticIcon
                }
            }

            QQC2.Label {
                visible: page.cfg_icon === ""
                opacity: 0.7
                text: page.cfg_recentFiles ? i18n("Automatic") : i18n("Automatic, from the folder")
            }

            QQC2.Menu {
                id: iconMenu

                QQC2.MenuItem {
                    text: i18n("Choose…")
                    icon.name: "document-open-folder"
                    onClicked: iconDialog.open()
                }
                QQC2.MenuItem {
                    text: i18n("Automatic")
                    icon.name: "edit-undo"
                    enabled: page.cfg_icon !== ""
                    onClicked: page.cfg_icon = ""
                }
            }
        }

        QQC2.ComboBox {
            id: viewBox
            visible: !page.dockOnSide
            Kirigami.FormData.label: i18n("Open as:")
            model: [i18n("Fan"), i18n("Grid"), i18n("Automatic")]
        }

        // A dock down the side of the screen always gets the grid (the choice above is kept for
        // when the dock goes back to the top or bottom)
        ColumnLayout {
            visible: page.dockOnSide
            Kirigami.FormData.label: i18n("Open as:")
            QQC2.Label {
                text: i18n("Grid")
            }
            QQC2.Label {
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                wrapMode: Text.Wrap
                font: Kirigami.Theme.smallFont
                opacity: 0.7
                text: i18n("The fan needs a dock along the top or bottom of the screen.")
            }
        }

        // Tilted text is never quite as sharp as level text
        QQC2.ComboBox {
            visible: !page.dockOnSide && viewBox.currentIndex !== 1   // only the fan has them
            Kirigami.FormData.label: i18n("Fan labels:")
            model: [i18n("Follow the curve"), i18n("Straight")]
            currentIndex: page.cfg_straightLabels ? 1 : 0
            onActivated: index => page.cfg_straightLabels = index === 1
        }

        QQC2.ComboBox {
            id: subfolderBox
            enabled: !page.cfg_recentFiles   // Recent Files only lists files
            Kirigami.FormData.label: i18n("Folders open:")
            model: [i18n("In place (grid) / in file manager (fan)"), i18n("Stacked on top")]
        }

        QQC2.ComboBox {
            id: sortBox
            Kirigami.FormData.label: i18n("Sort by:")
            model: [i18n("Newest first"), i18n("Name")]
        }

        QQC2.SpinBox {
            id: maxSpin
            visible: !page.dockOnSide   // no fan on a side dock
            Kirigami.FormData.label: i18n("Items in the fan:")
            from: 3
            to: 20
        }

        QQC2.CheckBox {
            id: previewsBox
            Kirigami.FormData.label: i18n("Previews:")
            text: i18n("Show previews of files")
        }

        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            opacity: 0.7
            text: i18n("Pictures, PDF pages, video frames, album art and office documents. When off, every file shows its plain icon.")
        }

        QQC2.CheckBox {
            id: newBox
            enabled: !page.cfg_recentFiles
            Kirigami.FormData.label: i18n("New files:")
            text: i18n("Mark files added since you last looked")
        }

        QQC2.CheckBox {
            id: bounceBox
            enabled: !page.cfg_recentFiles
            Kirigami.FormData.label: i18n("Downloads:")
            text: i18n("Bounce when a download finishes")
        }
    }

    // The icon picker KDE uses for its own panel buttons (it can also take a picture file)
    KIconThemes.IconDialog {
        id: iconDialog
        onIconNameChanged: iconName => {
            if (iconName)
                page.cfg_icon = iconName;
        }
    }

    readonly property bool dockOnSide: Plasmoid.location === PlasmaCore.Types.LeftEdge
                                       || Plasmoid.location === PlasmaCore.Types.RightEdge

    // Same choice the widget makes when no icon is set
    readonly property string automaticIcon: {
        if (page.cfg_recentFiles)
            return "document-open-recent";
        const name = folderField.text.replace(/\/$/, "").split("/").pop();
        switch (folderField.text === "" ? "Downloads" : name) {
        case "Downloads": return "folder-download";
        case "Documents": return "folder-documents";
        case "Pictures": return "folder-pictures";
        case "Music": return "folder-music";
        case "Videos": return "folder-videos";
        case "Desktop": return "user-desktop";
        default: return "folder";
        }
    }

    FolderDialog {
        id: picker
        onAccepted: folderField.text = decodeURIComponent(selectedFolder.toString().replace(/^file:\/\//, ""))
    }
}
