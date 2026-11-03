// SPDX-License-Identifier: MIT
import QtQuick

AudioMenu {
    id: optionsPopup
    required property var controller
    property string recordId: ""
    readonly property var entries: recordId ? controller.optionEntries(recordId) : []
    objectName: "audioDeviceOptionsMenu"
    function openFor(id, anchor) { recordId = id; if (entries.length) popup(anchor,0,anchor.height); }
    onEntriesChanged: if (!entries.length && visible) Qt.callLater(close)
    Instantiator {
        model: optionsPopup.entries.length
        delegate: AudioMenuItem {
            required property int index
            readonly property var entry: optionsPopup.entries[index] || {label:"",section:true}
            objectName: "audioOption:" + (entry?.key || index)
            section: !!entry?.section
            text: entry?.label || ""
            checkable: !!entry?.checkable
            checked: !!entry?.checked
            icon.name: entry?.icon || ""
            onTriggered: optionsPopup.controller.activateOption(optionsPopup.recordId,entry)
        }
        onObjectAdded: (index, object) => optionsPopup.insertItem(index,object)
        onObjectRemoved: (index, object) => optionsPopup.removeItem(object)
    }
}
