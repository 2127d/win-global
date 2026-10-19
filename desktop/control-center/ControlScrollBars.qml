// SPDX-License-Identifier: MIT
import QtQuick
import QtQml.Models
import QtQuick.Templates as T

// Only observe the active menu's visual tree. Keep scrollbar visibility and
// geometry intact: native pages use those to scroll focused controls into view.
QtObject {
    id: observer
    property Item targetItem

    readonly property Binding hideBar: Binding {
        target: observer.targetItem
        property: "opacity"
        when: observer.targetItem instanceof T.ScrollBar
        value: 0
        restoreMode: Binding.RestoreBindingOrValue
    }

    readonly property Instantiator descendants: Instantiator {
        model: observer.targetItem ? observer.targetItem.children : []
        // Load each observer lazily to avoid a recursive QML type definition.
        delegate: Loader {
            required property var modelData
            source: "ControlScrollBars.qml"
            onLoaded: item.targetItem = Qt.binding(() => modelData)
        }
    }
}
