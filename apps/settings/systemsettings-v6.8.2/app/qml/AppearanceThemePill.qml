// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtQuick.Controls as C
import QtQuick.Templates as T
import org.kde.kirigami as K

T.ComboBox {
    id: control
    required property Item backdropItem
    implicitWidth: 240
    implicitHeight: 42
    padding: 14
    spacing: 8
    leftPadding: padding + (mirrored ? indicator.width + spacing : 0)
    rightPadding: padding + (mirrored ? 0 : indicator.width + spacing)
    hoverEnabled: true
    wheelEnabled: false
    textRole: "name"
    Accessible.name: qsTr("Global theme and saved looks")
    background: HubPanel {
        radius: height / 2
        tint: control.down ? .75 : control.hovered ? .6 : .35
        emphasized: control.visualFocus || control.popup.visible
        Behavior on color { ColorAnimation { duration: 120 } }
    }
    contentItem: C.Label {
        text: control.displayText
        font: control.font
        color: K.Theme.textColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
        elide: Text.ElideRight
    }
    indicator: K.Icon {
        source: control.popup.visible ? "go-up" : "go-down"
        width: 16; height: 16
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: (control.height - height) / 2
    }
    delegate: C.ItemDelegate {
        required property var model
        required property int index
        width: ListView.view.width
        implicitHeight: 42
        text: model[control.textRole]
        highlighted: control.highlightedIndex === index
        hoverEnabled: true
        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
        background: HubPanel {
            radius: 12
            visible: parent.highlighted || parent.hovered
            color: Qt.alpha(K.Theme.highlightColor, .20)
            emphasized: parent.activeFocus
        }
        contentItem: C.Label {
            text: parent.text
            font: parent.font
            color: K.Theme.textColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
    popup: T.Popup {
        id: menu
        objectName: "appearanceThemePopup"
        parent: control
        popupType: T.Popup.Item
        x: 0
        y: control.height + 6
        width: control.width
        padding: 8
        topMargin: 0
        bottomMargin: 8
        implicitHeight: Math.min(list.contentHeight + topPadding + bottomPadding,
                                 Math.max(58, Math.min(360, control.Window.height - control.mapToItem(null, 0, control.height).y - 16)))
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent
        onOpened: { list.forceActiveFocus(); surface.updateSampleOrigin(); }
        contentItem: ListView {
            id: list
            objectName: "appearanceThemeOptions"
            clip: true
            Keys.forwardTo: [control]
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            C.ScrollBar.vertical: C.ScrollBar { policy: C.ScrollBar.AsNeeded }
        }
        background: Item {
            id: surface
            objectName: "appearanceThemePopupSurface"
            property point sampleOrigin: Qt.point(0, 0)
            function updateSampleOrigin() { sampleOrigin = mapToItem(control.backdropItem, 0, 0); }
            Connections {
                target: menu.visible ? surface.Window.window : null
                function onAfterAnimating() { surface.updateSampleOrigin(); }
            }
            ShaderEffectSource {
                id: backdrop
                visible: false
                sourceItem: menu.visible && appearancePreview.shaderPreviews ? control.backdropItem : null
                live: menu.visible
                hideSource: false
                smooth: true
            }
            ShaderEffect {
                objectName: "appearanceThemePopupGlass"
                anchors.fill: parent
                visible: appearancePreview.activeMaterial !== "solid" && appearancePreview.shaderPreviews
                property var source: backdrop
                property vector2d surfaceSize: Qt.vector2d(width, height)
                property vector2d sceneSize: Qt.vector2d(control.backdropItem.width, control.backdropItem.height)
                property vector4d sceneRect: Qt.vector4d(surface.sampleOrigin.x / sceneSize.x, surface.sampleOrigin.y / sceneSize.y, width / sceneSize.x, height / sceneSize.y)
                property real cornerRadius: 20
                property real refraction: appearancePreview.activeMaterial === "glass" ? 1 : 0
                fragmentShader: "qrc:/settings-hub/desktop-preview.frag.qsb"
            }
            HubPanel {
                anchors.fill: parent
                radius: 20
                tint: .45
                color: Qt.alpha(K.Theme.backgroundColor, appearancePreview.activeMaterial === "solid" || !appearancePreview.shaderPreviews ? 1 : .87)
            }
        }
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }
    }
}
