// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC
import "../details" as Details

Details.DetailNavigationStack {
    id: stack
    property Component sourceComponent
    property int selectedIndex: 0
    property Component displayedComponent: null
    property int displayedIndex: 0
    implicitHeight: currentItem?.implicitHeight ?? 0

    // Coalesce tab/model changes, including tabs disappearing after an update.
    // An owned timer also cancels pending work when Weather is unloaded.
    onSourceComponentChanged: updatePage.restart()
    onSelectedIndexChanged: updatePage.restart()
    Timer {
        id: updatePage
        interval: 0
        onTriggered: {
            if (!stack.sourceComponent) {
                stack.clear(QQC.StackView.Immediate);
            } else if (stack.sourceComponent !== stack.displayedComponent || stack.empty) {
                const dimensions = {
                    width: Qt.binding(() => stack.width),
                    height: Qt.binding(() => stack.height)
                };
                if (stack.empty) stack.push(stack.sourceComponent, dimensions, QQC.StackView.Immediate);
                else stack.replace(stack.sourceComponent, dimensions,
                    stack.selectedIndex >= stack.displayedIndex ? QQC.StackView.PushTransition : QQC.StackView.PopTransition);
            }
            stack.displayedComponent = stack.sourceComponent;
            stack.displayedIndex = stack.selectedIndex;
        }
    }
}
