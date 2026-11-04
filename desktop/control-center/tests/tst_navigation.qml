// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtTest
import "../details" as Details

Item {
    id: host
    width: 420; height: 400
    Component { id: page; Item { implicitHeight: 240 } }
    Component { id: stackComponent; Details.DetailNavigationStack { width: 420; height: 400; motionDuration: 240 } }
    Component {
        id: deckComponent
        Details.DetailPageDeck {
            width: 420; height: 400; motionDuration: 240
            pages: [Item { implicitHeight: 240 }, Item { implicitHeight: 360 }]
        }
    }
    TestCase {
        name: "DirectionalNavigation"
        when: windowShown
        function test_forward_and_back() {
            const stack = createTemporaryObject(stackComponent, host);
            stack.push(page, {}, QQC2.StackView.Immediate);
            const overview = stack.currentItem;
            stack.push(page);
            const detail = stack.currentItem;
            wait(60);
            verify(stack.busy);
            verify(detail.x > 0 && detail.x < stack.width);
            verify(overview.x < 0 && overview.x > -stack.width);
            tryCompare(stack, "busy", false);
            compare(detail.x, 0);
            stack.pop();
            wait(60);
            verify(overview.x < 0 && overview.x > -stack.width);
            verify(detail.x > 0 && detail.x < stack.width);
            tryCompare(stack, "busy", false);
            compare(overview.x, 0);
            compare(overview.opacity, 1);
            verify(stack.clip);
        }
        function test_deck_preserves_pages_and_intrinsic_size() {
            const deck = createTemporaryObject(deckComponent, host);
            compare(deck.currentItem, deck.pages[0]);
            compare(deck.implicitHeight, 240);
            deck.currentIndex = 1;
            wait(60);
            verify(deck.pages[1].x > 0);
            verify(deck.pages[0].x < 0);
            compare(deck.implicitHeight, 360);
            tryCompare(deck, "busy", false);
            deck.currentIndex = 0;
            wait(60);
            verify(deck.pages[0].x < 0);
            verify(deck.pages[1].x > 0);
            tryCompare(deck, "busy", false);
            compare(deck.currentItem, deck.pages[0]);
            compare(deck.pages[0].x, 0);
            verify(!deck.pages[1].visible);
        }
        function test_fast_back_and_reduced_motion() {
            const deck = createTemporaryObject(deckComponent, host);
            deck.currentIndex = 1;
            wait(40);
            deck.currentIndex = 0;
            tryCompare(deck, "busy", false);
            compare(deck.currentItem, deck.pages[0]);
            compare(deck.currentItem.x, 0);
            deck.motionDuration = 0;
            deck.currentIndex = 1;
            tryCompare(deck, "busy", false);
            compare(deck.currentItem, deck.pages[1]);
            compare(deck.currentItem.x, 0);
        }
    }
}
