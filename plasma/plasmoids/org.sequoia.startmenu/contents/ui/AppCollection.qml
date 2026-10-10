// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick

Grid {
    id: collection
    required property var menu
    property var sourceModel
    property bool populated: true
    property int limit: -1
    property bool listStyle: false
    property int columnCount: listStyle ? 1 : menu.configuration.gridColumns
    columns: columnCount
    spacing: 4
    Repeater {
        model: collection.populated ? collection.sourceModel : null
        delegate: Loader {
            id: slot
            required property int index
            required property var model
            active: collection.limit < 0 || index < collection.limit
            visible: active
            width: active ? (collection.width - collection.spacing * (collection.columns - 1)) / collection.columns : 0
            sourceComponent: AppTile {
                menu: collection.menu
                sourceModel: collection.sourceModel
                entryIndex: slot.index
                entry: slot.model
                listStyle: collection.listStyle
            }
        }
    }
}
