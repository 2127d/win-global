// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick

ActivityPill {
    model: ListModel {
        Component.onCompleted: append([
            {name: "Default", id: "default", current: true},
            {name: "Gaming", id: "gaming", current: false}
        ])
    }
}
