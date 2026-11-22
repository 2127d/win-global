// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.sequoia.components 1.0 as S
import org.kde.plasma.landingpage.kcm

S.ThemePreviewCard {
    id: card
    required property Item backdropItem
    property string packageId
    property int variant
    signal accepted(string packageId)
    caption: metadata.name
    previewSource: metadata.preview
    LookAndFeelMetaData { id: metadata; packageId: card.packageId }
    onChangeRequested: chooser.visible ? chooser.close() : chooser.open()
    // Discover installed packs only when the user opens this picker.
    property var packModel: null
    Component { id: modelComponent; LookAndFeelModel { variant: card.variant } }
    S.PopupMenu {
        id: chooser
        objectName: card.objectName + "Menu"
        theme: card.theme; trigger: card.changeButton; backdropItem: card.backdropItem
        width: Math.min(340, card.backdropItem.width - 24)
        onAboutToShow: { if (!card.packModel) card.packModel = modelComponent.createObject(card); }
        Instantiator {
            model: card.packModel
            delegate: S.MenuItem {
                id: option
                required property string packageId
                required property string name
                required property url preview
                theme: card.theme
                text: name; checkable: true; checked: packageId === card.packageId
                implicitHeight: 64
                contentItem: RowLayout {
                    spacing: 10
                    Image { Layout.preferredWidth: 72; Layout.preferredHeight: 45; source: option.preview; fillMode: Image.PreserveAspectFit }
                    C.Label { Layout.fillWidth: true; text: option.text; color: card.theme.textColor; elide: Text.ElideRight }
                }
                background: S.MenuHighlight {
                    theme: card.theme; highlighted: option.highlighted; hovered: option.hovered
                    pressed: option.down; focused: option.visualFocus
                }
                onTriggered: card.accepted(packageId)
            }
            onObjectAdded: (index, object) => chooser.insertItem(index, object)
            onObjectRemoved: (index, object) => chooser.removeItem(object)
        }
    }
}
