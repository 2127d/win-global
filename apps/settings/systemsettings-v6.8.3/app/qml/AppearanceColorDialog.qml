// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.sequoia.components 1.0 as S
S.ColorDialog {
    title: i18n("Choose accent color")
    namePrefix: "appearanceCustomAccent"
    theme: AppearanceLibraryTheme {}
    property ScrollEdgeFade edgeFade: ScrollEdgeFade { viewport: scrollViewport; fadeSize: 20 }
    onAccepted: appearancePreview.stageAccent(String(selectedColor))
}
