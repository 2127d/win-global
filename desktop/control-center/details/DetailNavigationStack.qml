// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.StackView {
    id: stack
    property int motionDuration: Kirigami.Units.longDuration
    clip: true

    // Forward pages enter from the right; Back retraces the same path.
    // Animate position only so text and controls remain crisp throughout.
    pushEnter: Transition {
        NumberAnimation { property: "x"; from: stack.width; to: 0; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
    pushExit: Transition {
        NumberAnimation { property: "x"; from: 0; to: -stack.width; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
    popEnter: Transition {
        NumberAnimation { property: "x"; from: -stack.width; to: 0; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
    popExit: Transition {
        NumberAnimation { property: "x"; from: 0; to: stack.width; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
    replaceEnter: Transition {
        NumberAnimation { property: "x"; from: stack.width; to: 0; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
    replaceExit: Transition {
        NumberAnimation { property: "x"; from: 0; to: -stack.width; duration: stack.motionDuration; easing.type: Easing.OutCubic }
    }
}
