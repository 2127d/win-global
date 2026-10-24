// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: root
    property string text
    property var model
    property string textRole: "name"
    property string valueRole: "value"
    property var selectedValue
    readonly property int count: model ? model.count : 0
    readonly property int currentIndex: indexOfValue(selectedValue)
    readonly property var currentValue: currentIndex >= 0 ? model.get(currentIndex)[valueRole] : undefined
    readonly property string currentText: currentIndex >= 0 ? model.get(currentIndex)[textRole] : ""
    property bool expanded: false
    property var viewport: null
    readonly property real expansionHeight: reveal.height
    readonly property real collapsedHeight: heading.implicitHeight
    signal activated(int index)
    function indexOfValue(value) {
        for (let i = 0; i < count; ++i) {
            if (model.get(i)[valueRole] === value) return i;
        }
        return -1;
    }
    function focusOption(index) {
        const option = choices.itemAt(Math.max(0, Math.min(count - 1, index)));
        if (option) option.forceActiveFocus(Qt.TabFocusReason);
    }
    function choose(index) {
        activated(index);
        expanded = false;
        heading.forceActiveFocus(Qt.OtherFocusReason);
    }
    function ensureVisible() {
        const view = viewport;
        if (!view) return;
        view.cancelFlick();
        const y = root.mapToItem(view.contentItem, 0, 0).y;
        const target = Math.min(y, Math.max(view.contentY, y + root.height - view.height));
        view.contentY = Math.max(view.originY, Math.min(target, view.contentHeight - view.height));
    }
    Layout.fillWidth: true
    implicitHeight: heading.implicitHeight + reveal.height

    Button {
        id: heading
        objectName: root.objectName + "Toggle"
        width: parent.width
        implicitHeight: Math.max(46, headingContent.implicitHeight + 24)
        padding: 12
        text: root.text
        Accessible.name: root.text + ": " + root.currentText
        Accessible.description: root.expanded ? i18n("Choices expanded. Activate to collapse.") : i18n("Activate to show choices.")
        onClicked: root.expanded = !root.expanded
        Keys.onDownPressed: {
            root.expanded = true;
            Qt.callLater(root.focusOption, root.currentIndex);
        }
        Keys.onEscapePressed: root.expanded = false
        background: Rectangle {
            radius: 13
            color: heading.down ? Qt.alpha(Kirigami.Theme.textColor, 0.12) : heading.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.05) : "transparent"
            border.width: heading.visualFocus ? 2 : 0
            border.color: Kirigami.Theme.highlightColor
            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
        }
        contentItem: RowLayout {
            id: headingContent
            spacing: 10
            Label { text: root.text; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Label {
                text: root.currentText
                Layout.maximumWidth: heading.availableWidth * 0.46
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignRight
                color: Kirigami.Theme.disabledTextColor
            }
            Kirigami.Icon {
                source: "go-down-symbolic"
                isMask: true
                Layout.preferredWidth: 16; Layout.preferredHeight: 16
                rotation: root.expanded ? 180 : 0
                Behavior on rotation { NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad } }
            }
        }
    }

    Item {
        id: reveal
        anchors.top: heading.bottom
        width: parent.width
        height: root.expanded ? options.implicitHeight + 8 : 0
        clip: true
        enabled: root.expanded
        visible: height > 0
        Behavior on height { NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad } }
        ColumnLayout {
            id: options
            width: parent.width - 16
            x: 8
            spacing: 4
            Repeater {
                id: choices
                model: root.model
                delegate: ItemDelegate {
                    id: option
                    required property int index
                    required property var model
                    objectName: root.objectName + "Option" + index
                    Layout.fillWidth: true
                    implicitHeight: Math.max(40, optionContent.implicitHeight + 20)
                    padding: 10
                    text: model[root.textRole]
                    highlighted: root.currentIndex === index
                    Accessible.role: Accessible.RadioButton
                    Accessible.checked: highlighted
                    Accessible.name: text
                    onClicked: root.choose(index)
                    Keys.onReturnPressed: root.choose(index)
                    Keys.onEnterPressed: root.choose(index)
                    Keys.onDownPressed: root.focusOption(index + 1)
                    Keys.onUpPressed: root.focusOption(index - 1)
                    Keys.onEscapePressed: {
                        root.expanded = false;
                        heading.forceActiveFocus();
                    }
                    background: Rectangle {
                        radius: 12
                        color: option.highlighted ? Qt.alpha(Kirigami.Theme.highlightColor, 0.14) : option.down ? Qt.alpha(Kirigami.Theme.textColor, 0.10) : option.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.05) : "transparent"
                        border.width: option.visualFocus ? 2 : 0
                        border.color: Kirigami.Theme.highlightColor
                        Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }
                    }
                    contentItem: RowLayout {
                        id: optionContent
                        spacing: 10
                        Label { text: option.text; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        Kirigami.Icon {
                            source: "dialog-ok-apply"
                            isMask: true
                            Layout.preferredWidth: 18; Layout.preferredHeight: 18
                            opacity: option.highlighted ? 1 : 0
                            color: Kirigami.Theme.highlightColor
                        }
                    }
                }
            }
        }
    }
    // Reveal choices that opened near the bottom of either Settings viewport.
    Timer {
        interval: Kirigami.Units.longDuration + 20
        running: root.expanded
        onTriggered: root.ensureVisible()
    }
}
