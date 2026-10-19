// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.PlasmoidHeading {
    objectName: "sequoiaDetailHeading"
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    leftPadding: 10
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    background: DetailSurface { radius: 16; elevated: true }
}
