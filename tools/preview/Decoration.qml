import QtQuick
import org.kde.kwin.private.kdecoration as KDecoration

Rectangle {
    width: 1000; height: 550
    property bool alphaProbe: false
    color: alphaProbe ? "transparent" : (previewDark ? "#17181b" : "#e5e8ef")
    property bool pluginLoaded: bridge.valid
    KDecoration.Bridge { id: bridge; plugin: previewPlugin }
    KDecoration.Settings { id: decorationSettings; bridge: bridge.bridge }
    Text {
        x: 55; y: 35
        text: "SEQUOIA ELEVEN / WINDOW DECORATION"
        color: previewDark ? "#c0c5ce" : "#525865"
        font.pixelSize: 12; font.letterSpacing: 2
    }
    KDecoration.Decoration {
        id: window
        x: 50; y: 95; width: 900; height: 400
        bridge: bridge.bridge
        settings: decorationSettings
        windowColor: alphaProbe ? "transparent" : (previewDark ? "#252629" : "#f2f3f5")
        Component.onCompleted: {
            client.caption = "Sequoia Eleven"
            client.active = true
            client.closeable = true
            client.minimizable = true
            client.maximizable = true
        }
        Text {
            x: 95; y: 130
            text: "Soft edges. Clear focus."
            color: previewDark ? "#f2f3f5" : "#24262b"
            font.pixelSize: 30; font.weight: Font.DemiBold
        }
        Text {
            x: 95; y: 190
            width: 670
            text: "A compact title bar, centered caption, and fixed-size traffic-light controls.\nRendered by the compiled Sequoia Eleven KWin decoration plugin."
            color: previewDark ? "#adb0b8" : "#666b75"
            font.pixelSize: 15; lineHeight: 1.6
        }
    }
}
