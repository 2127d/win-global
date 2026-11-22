// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
import org.kde.kirigami as K
import org.sequoia.components 1.0 as S

HubPanel {
    id: card
    required property Item backdropItem
    property bool canApply: true
    readonly property var currentPack: appearancePreview.iconPacks.find(pack => pack.id === appearancePreview.iconPack)
    implicitWidth: selector.implicitWidth + samples.implicitWidth + more.implicitWidth + 2 * contents.spacing + 24
    implicitHeight: contents.implicitHeight + 24
    radius: height / 2
    tint: .42
    onVisibleChanged: if (visible) appearancePreview.refreshIconPacks()
    FontMetrics {
        id: nameMetrics
        font.family: selector.font.family
        font.pixelSize: selector.font.pixelSize
        // Selected menu entries are semibold; reserve enough for any selection.
        font.weight: Font.DemiBold
    }
    RowLayout {
        id: contents
        anchors.fill: parent; anchors.margins: 12; spacing: 12
        S.ThemeSelector {
            id: selector
            objectName: "appearanceIconPackSelector"
            theme: AppearanceLibraryTheme {}
            backdropItem: card.backdropItem
            readonly property real nameWidth: {
                let widest = nameMetrics.advanceWidth(displayText);
                for (const pack of appearancePreview.iconPacks)
                    widest = Math.max(widest, nameMetrics.advanceWidth(pack.name));
                return Math.ceil(widest);
            }
            implicitWidth: nameWidth + leftPadding + rightPadding
            Layout.preferredWidth: Math.min(implicitWidth, Math.max(120, contents.width - more.implicitWidth - 120 - 2 * contents.spacing))
            Layout.minimumWidth: 0
            minimumPopupWidth: implicitWidth
            model: appearancePreview.iconPacks
            currentIndex: appearancePreview.iconPacks.findIndex(pack => pack.id === appearancePreview.iconPack)
            displayText: currentIndex < 0 ? (count ? appearancePreview.iconPack : i18n("No icon packs available")) : currentText
            Accessible.name: i18n("Icon pack")
            Accessible.description: i18n("Applies immediately")
            C.ToolTip.visible: hovered && !popup.visible
            C.ToolTip.text: i18n("Applies immediately")
            C.ToolTip.delay: 700
            enabled: card.canApply && count > 0
            onActivated: index => {
                appearancePreview.applyIconPack(model[index].id);
                // Keep the selector truthful if a removed or locked pack fails.
                currentIndex = Qt.binding(() => appearancePreview.iconPacks.findIndex(pack => pack.id === appearancePreview.iconPack));
            }
        }
        C.ScrollView {
            id: sampleScroll
            objectName: "appearanceIconPackSamples"
            property ScrollEdgeFade edgeFade: ScrollEdgeFade {
                objectName: "appearanceIconPackFade"
                viewport: sampleScroll.contentItem
                fadeSize: 16
            }
            Layout.preferredWidth: samples.implicitWidth
            Layout.maximumWidth: samples.implicitWidth
            Layout.minimumWidth: 32
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            padding: 0; background: null; clip: true
            contentHeight: availableHeight
            C.ScrollBar.horizontal.policy: C.ScrollBar.AlwaysOff
            C.ScrollBar.vertical.policy: C.ScrollBar.AlwaysOff
            Row {
                id: samples
                spacing: 12
                Repeater {
                    model: card.currentPack ? card.currentPack.samples : ["folder", "user-home", "user-trash", "text-x-generic", "systemsettings"].map(icon => ({icon: icon, source: icon}))
                    K.Icon {
                        required property var modelData
                        objectName: "appearanceIconPackSample-" + modelData.icon
                        width: 32; height: 32
                        source: modelData.source
                        Accessible.ignored: true
                    }
                }
            }
        }
        S.CircleButton {
            id: more
            objectName: "appearanceIconPackMore"
            theme: AppearanceLibraryTheme {}
            label: i18n("More icon settings"); ellipsis: true
            onClicked: navigation.open("kcm_icons")
        }
    }
}
