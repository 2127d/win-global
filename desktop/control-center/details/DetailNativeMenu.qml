// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.plasma.extras as PlasmaExtras

PlasmaExtras.Menu {
    id: menu
    property QtObject sequoiaLifetime: DetailPopupLifetime {
        popup: menu
        owner: menu.visualParent
    }
}
