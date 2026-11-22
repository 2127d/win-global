// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigami as Kirigami

// Plasma's ResizeHandle currently enforces minimum sizes but ignores maxima.
// For a content-sized widget, consume the gesture before that native handler
// writes geometry. Keep its existing handle, shadow, animation and red feedback.
MouseArea {
    id: guard
    objectName: "sequoiaFixedSizeHandleGuard"
    required property var nativeHandle
    parent: nativeHandle
    anchors.fill: parent
    z: 1
    acceptedButtons: Qt.LeftButton
    preventStealing: true
    cursorShape: Qt.ForbiddenCursor
    readonly property color blockedColor: Kirigami.Theme.negativeTextColor
    readonly property var surface: nativeHandle.children.find(child =>
        child.shadow !== undefined) ?? null

    Binding {
        target: guard.surface
        property: "color"
        value: guard.blockedColor
        when: guard.pressed && guard.surface !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
}
