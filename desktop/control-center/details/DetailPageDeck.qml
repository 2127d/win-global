// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2

DetailNavigationStack {
    id: deck
    // Keep existing view instances and their native models alive across navigation.
    property list<Item> pages
    property int currentIndex: 0
    property int previousIndex: 0
    property bool ready: false
    implicitHeight: currentItem?.implicitHeight ?? 0

    function selectPage() {
        if (!ready || !pages[currentIndex] || currentItem === pages[currentIndex]) return;
        const operation = currentIndex >= previousIndex ? QQC2.StackView.PushTransition : QQC2.StackView.PopTransition;
        const dimensions = {width: Qt.binding(() => deck.width), height: Qt.binding(() => deck.height)};
        if (empty) push(pages[currentIndex], dimensions, QQC2.StackView.Immediate);
        else replace(pages[currentIndex], dimensions, operation);
        previousIndex = currentIndex;
    }
    onCurrentIndexChanged: selectPage()
    Component.onCompleted: { ready = true; selectPage(); }
}
