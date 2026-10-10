// SPDX-License-Identifier: MIT
import QtQuick
import QtTest
import org.sequoia.login 1.0

Item {
    id: scene
    width: 1280; height: 800
    Image { anchors.fill: parent; source: "backdrop.svg"; fillMode: Image.PreserveAspectCrop }
    WallpaperGlass {
        id: panel
        x: 160; y: 140; width: 240; height: 180; radius: 24
        source: Qt.resolvedUrl("backdrop.svg"); sceneSize: Qt.size(scene.width, scene.height)
        sampleRect: Qt.rect(x, y, width, height); blurRadius: 24; tint: "transparent"
    }
    WallpaperGlass {
        id: circle
        x: 1040; y: 660; width: 64; height: 64; radius: 32
        source: panel.source; sceneSize: panel.sceneSize
        sampleRect: Qt.rect(x, y, width, height); blurRadius: 24; tint: "transparent"
    }
    TestCase {
        name: "WallpaperGlass"
        when: windowShown
        function test_blur_and_clipping() {
            waitForRendering(scene);
            const image = grabImage(scene);
            function spread(x, y, count) {
                let low = 255, high = 0;
                for (let i = 0; i < count; ++i) { const value = image.red(x + i, y); low = Math.min(low, value); high = Math.max(high, value); }
                return high - low;
            }
            verify(spread(32, 100, 32) > 100, "Sharp wallpaper must retain its stripe detail");
            verify(spread(200, 220, 32) < 12, "Card background must be blurred, not just tinted");
            verify(spread(1056, 692, 24) < 12, "Power circle must have real blur too");
            verify(image.blue(220, 220) > image.green(220, 220) + 60, "Card samples the blue side");
            verify(image.green(1072, 692) > image.blue(1072, 692) + 60, "Power circle samples its own green region");
            // The top-left of a rounded surface stays sharp, outside its mask.
            verify(spread(1040, 660, 8) > 100, "Circle corners must not contain blur");
        }
        function test_moving_surface_updates_sample() {
            circle.x = 480; waitForRendering(scene);
            const image = grabImage(scene);
            verify(image.blue(512, 692) > image.green(512, 692) + 60);
            circle.x = 1040;
        }
        function test_relative_wallpaper_url() {
            // Installed QML uses a relative URL; previews used to hide this by
            // replacing it with an absolute file URL before loading the theme.
            panel.source = "backdrop.svg";
            waitForRendering(scene);
            const image = grabImage(scene);
            let low = 255, high = 0;
            for (let x = 200; x < 232; ++x) {
                low = Math.min(low, image.red(x, 220));
                high = Math.max(high, image.red(x, 220));
            }
            verify(high - low < 12, "Relative wallpaper URLs must render blur instead of leaving the stripes sharp");
            verify(image.blue(220, 220) > image.green(220, 220) + 60);
            panel.source = Qt.resolvedUrl("backdrop.svg");
        }
    }
}
