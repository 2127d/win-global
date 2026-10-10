import QtQuick
import org.kde.ksvg as KSvg

Rectangle {
    id: root
    width: 1440; height: 960
    property bool dark: initialDark
    property color ink: dark ? "#f0f3fa" : "#202633"
    property color muted: dark ? "#a9b4c8" : "#616b7c"
    property color accent: dark ? "#8db3ff" : "#2463d4"
    property string themePath: projectRoot + "/plasma/desktoptheme/sequoia-eleven-" + (dark ? "dark" : "light")
    color: dark ? "#131925" : "#edf0f6"

    component Label: Text {
        color: root.ink
        font.family: "Nunito"
        font.pixelSize: 15
        renderType: Text.QtRendering
    }
    component Surface: KSvg.FrameSvgItem {
        id: surface
        property string asset: "dialogs/background"
        property string state: ""
        imagePath: root.themePath + "/" + asset + ".svg"
        prefix: state
        KSvg.FrameSvgItem {
            anchors.fill: parent; anchors.margins: -12; z: -1
            imagePath: surface.imagePath; prefix: "shadow"
            visible: surface.asset === "dialogs/background" || surface.asset === "widgets/background" || surface.asset === "widgets/panel-background"
        }
    }
    component Action: Item {
        property string label
        property string state: "normal"
        width: 116; height: 40
        Surface { anchors.fill: parent; asset: "widgets/button"; state: parent.state }
        Label { anchors.centerIn: parent; text: parent.label; font.pixelSize: 13 }
    }

    Label { x: 64; y: 38; text: "SEQUOIA ELEVEN"; font.pixelSize: 13; font.letterSpacing: 3; font.weight: Font.DemiBold }
    Label { x: 64; y: 82; text: "A little clarity. A little calm."; font.pixelSize: 38; font.weight: Font.DemiBold }
    Label { x: 66; y: 140; text: "Windows-inspired precision meets the softness of macOS."; color: root.muted; font.pixelSize: 17 }
    Action {
        x: 1210; y: 48; width: 164; label: root.dark ? "Switch to light" : "Switch to dark"
        MouseArea { anchors.fill: parent; onClicked: root.dark = !root.dark; cursorShape: Qt.PointingHandCursor }
    }

    Rectangle {
        x: 64; y: 202; width: 1312; height: 606; radius: 24; clip: true
        gradient: Gradient {
            GradientStop { position: 0; color: root.dark ? "#233453" : "#c7d7ee" }
            GradientStop { position: 1; color: root.dark ? "#473746" : "#e8d4c7" }
        }
        // The bundled adaptive wallpaper uses this geometry and palette; see tools/wallpaper.py.
        Rectangle { x: 800; y: -310; width: 500; height: 900; radius: 250; rotation: -34; color: root.dark ? "#514566" : "#b9bee1" }
        Rectangle { x: 900; y: -240; width: 360; height: 890; radius: 180; rotation: -34; color: root.dark ? "#735269" : "#d7bdd1" }
        Rectangle { x: 975; y: -210; width: 230; height: 890; radius: 115; rotation: -34; color: root.dark ? "#986451" : "#ecc4ae" }

        Surface {
            x: 50; y: 44; width: 500; height: 432
            Label { x: 26; y: 22; text: "Make yourself at home"; font.pixelSize: 23; font.weight: Font.DemiBold }
            Label { x: 26; y: 58; text: "Comfortable surfaces. Deliberate details."; color: root.muted }
            Surface {
                x: 24; y: 100; width: 452; height: 44; asset: "widgets/lineedit"; state: "base"
                Label { x: 16; anchors.verticalCenter: parent.verticalCenter; text: "Search applications, files, and more…"; color: root.muted; font.pixelSize: 14 }
            }
            Label { x: 26; y: 172; text: "YOUR SPACE"; font.pixelSize: 11; font.letterSpacing: 1.5; color: root.muted }
            Repeater {
                model: ["Projects", "Documents", "Pictures"]
                delegate: Item {
                    required property int index
                    required property string modelData
                    x: 24; y: 201 + index * 54; width: 452; height: 47
                    Surface { anchors.fill: parent; asset: "widgets/viewitem"; state: index === 0 ? "selected" : index === 1 ? "hover" : "normal" }
                    Rectangle { x: 14; y: 15; width: 21; height: 16; radius: 3; color: index === 0 ? (root.dark ? "#15213a" : "#ffffff") : root.accent; opacity: 0.85 }
                    Label { x: 50; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: index === 0 ? (root.dark ? "#15213a" : "#ffffff") : root.ink }
                    Label { x: 412; anchors.verticalCenter: parent.verticalCenter; text: "›"; font.pixelSize: 22; color: index === 0 ? (root.dark ? "#15213a" : "#ffffff") : root.muted }
                }
            }
            Label { x: 26; y: 388; text: "Everything has a place."; color: root.muted; font.pixelSize: 13 }
        }

        Surface {
            x: 580; y: 44; width: 388; height: 240
            Label { x: 24; y: 22; text: "Small details, considered"; font.pixelSize: 20; font.weight: Font.DemiBold }
            Label { x: 24; y: 57; text: "Soft corners · crisp focus · subtle borders"; color: root.muted; font.pixelSize: 13 }
            Action { x: 24; y: 98; width: 104; label: "Default" }
            Action { x: 140; y: 98; width: 104; label: "Hover"; state: "hover" }
            Action { x: 256; y: 98; width: 108; label: "Pressed"; state: "pressed" }
            Surface { x: 24; y: 160; width: 340; height: 48; asset: "widgets/lineedit"; state: "base" }
            Surface { x: 24; y: 160; width: 340; height: 48; asset: "widgets/lineedit"; state: "focus" }
            Label { x: 40; y: 173; text: "Keyboard focus you can see"; font.pixelSize: 14 }
        }
        Surface {
            x: 580; y: 304; width: 388; height: 172; asset: "widgets/background"
            Label { x: 24; y: 23; text: "Room to breathe"; font.pixelSize: 20; font.weight: Font.DemiBold }
            Label { x: 24; y: 62; width: 330; wrapMode: Text.WordWrap; text: "A cool neutral palette with a clear blue accent. Available in light and dark."; color: root.muted; lineHeight: 1.35 }
            Row {
                x: 24; y: 126; spacing: 8
                Repeater { model: [root.accent, root.dark ? "#303747" : "#ffffff", root.dark ? "#4b5569" : "#c7cedb", root.dark ? "#a9b4c8" : "#616b7c"]
                    delegate: Rectangle { required property color modelData; width: 20; height: 20; radius: 10; color: modelData }
                }
            }
        }
        Label { x: 1010; y: 66; text: "09:41"; font.pixelSize: 47; color: root.dark ? "#fff6f0" : "#483e52"; font.weight: Font.Light }
        Label { x: 1015; y: 128; text: "A calmer desktop"; font.pixelSize: 14; color: root.dark ? "#fff6f0" : "#483e52" }

        Surface {
            x: 322; y: 518; width: 668; height: 66; asset: "widgets/panel-background"
            Row {
                x: 17; y: 9; spacing: 8
                Repeater {
                    model: ["S", "F", "W", "T", "M", "+"]
                    delegate: Item {
                        required property int index
                        required property string modelData
                        width: 50; height: 48
                        // Pinned, focused, running, minimized, hovered running, pinned.
                        Surface { anchors.fill: parent; asset: "widgets/tasks"; state: ["", "focus", "normal", "minimized", "normal-hover", ""][index] }
                        Rectangle { anchors.centerIn: parent; width: 29; height: 29; radius: 8; color: ["#2463d4", "#438ea3", "#8068b5", "#586577", "#c37464", "#637490"][index]
                            Text { anchors.centerIn: parent; text: modelData; color: "white"; font.pixelSize: 15; font.weight: Font.DemiBold }
                        }
                    }
                }
            }
            Rectangle { x: 380; y: 18; width: 1; height: 30; color: root.muted; opacity: 0.25 }
            Surface { x: 394; y: 9; width: 130; height: 48; asset: "widgets/tabbar"; state: "south-active-tab" }
            Label { x: 405; y: 22; text: "Wi-Fi  ·  100%"; font.pixelSize: 13 }
            Label { x: 562; y: 15; text: "09:41"; font.pixelSize: 15; font.weight: Font.DemiBold }
            Label { x: 551; y: 36; text: "19 September"; font.pixelSize: 10; color: root.muted }
        }
    }
    Label { x: 68; y: 840; text: root.dark ? "01 / DARK" : "01 / LIGHT"; font.pixelSize: 11; font.letterSpacing: 2; color: root.muted }
    Label { x: 68; y: 872; text: "Actual theme SVGs rendered by KDE’s KSvg engine."; font.pixelSize: 16 }
    Label { x: 68; y: 906; text: "Component preview with illustrative content. Panel layout and application icons are configured separately."; font.pixelSize: 12; color: root.muted }
}
