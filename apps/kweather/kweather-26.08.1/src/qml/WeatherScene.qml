// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import org.kde.kirigami as Kirigami

Item {
    id: sky
    property string condition: ""
    property bool motion: true
    readonly property bool night: condition.indexOf("night") >= 0
    readonly property bool rain: /rain|showers|storm/.test(condition)
    readonly property bool snow: /snow|hail/.test(condition)
    readonly property bool clouds: /cloud|overcast|fog|mist/.test(condition) || rain || snow
    readonly property bool animate: motion && visible && Window.window && Window.window.visibility !== Window.Minimized && Kirigami.Units.longDuration > 0
    clip: true
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: sky.night ? "#10192f" : sky.rain ? "#263d54" : sky.snow ? "#496778" : "#265679"; Behavior on color { ColorAnimation { duration: Kirigami.Units.longDuration * 2 } } }
            GradientStop { position: 0.55; color: sky.night ? "#1b3047" : sky.rain ? "#375b72" : sky.snow ? "#678b9b" : "#498b9e"; Behavior on color { ColorAnimation { duration: Kirigami.Units.longDuration * 2 } } }
            GradientStop { position: 1; color: "#111f32" }
        }
    }
    // The sky is vector-painted once. Only the lightweight layers move.
    Canvas {
        id: glow
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections { target: sky; function onConditionChanged() { glow.requestPaint(); } }
        onPaint: {
            const c = getContext("2d"); c.reset();
            const x = width * 0.76, y = height * 0.14, r = width * 0.6;
            const g = c.createRadialGradient(x,y,0,x,y,r);
            g.addColorStop(0, sky.night ? "rgba(96,137,179,0.25)" : sky.clouds ? "rgba(145,186,196,0.28)" : "rgba(255,221,156,0.4)");
            g.addColorStop(1,"transparent"); c.fillStyle=g; c.fillRect(0,0,width,height);
        }
    }
    Repeater {
        model: sky.night ? 42 : 0
        Rectangle {
            required property int index
            x: ((index * 137.51) % 1000) / 1000 * sky.width
            y: ((index * 73.13) % 600) / 1000 * sky.height
            width: index % 4 === 0 ? 2 : 1; height: width; radius: width
            color: "#e1eeff"; opacity: 0.3 + (index % 5) * 0.1
            SequentialAnimation on opacity {
                running: sky.animate; loops: Animation.Infinite
                NumberAnimation { to: 0.18; duration: 2300 + index*80 }
                NumberAnimation { to: 0.75; duration: 3200 + index*60 }
            }
        }
    }
    Rectangle {
        visible: !sky.clouds
        x: sky.width * 0.77; y: sky.height * 0.12
        width: sky.night ? 48 : 72; height: width; radius: width/2
        color: sky.night ? "#f0edd9" : "#ffdfa4"; opacity: 0.85
        Rectangle { visible: sky.night; x: -7; y: -6; width: 45; height: 45; radius: 30; color: "#20364d" }
    }
    Repeater {
        model: sky.clouds ? 3 : 1
        Canvas {
            id: cloud
            required property int index
            width: sky.width * 1.4; height: sky.height * 0.48
            y: sky.height * (0.01 + index * 0.12)
            opacity: 0.3 - index*0.055
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const c=getContext("2d"); c.reset();
                for(let n=0;n<5;n++) {
                    const x=width*(n*0.23+0.03), y=height*0.5, r=height*0.48;
                    const g=c.createRadialGradient(x,y,0,x,y,r);
                    g.addColorStop(0,"rgba(215,232,237,0.6)"); g.addColorStop(1,"transparent");
                    c.fillStyle=g; c.fillRect(0,0,width,height);
                }
            }
            SequentialAnimation on x {
                running: sky.animate; loops: Animation.Infinite
                NumberAnimation { from: -sky.width*0.3; to: -sky.width*0.08; duration: 24000 + index*5000; easing.type: Easing.InOutSine }
                NumberAnimation { to: -sky.width*0.3; duration: 24000 + index*5000; easing.type: Easing.InOutSine }
            }
        }
    }
    Canvas {
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const c=getContext("2d"); c.reset();
            for(let layer=0;layer<3;layer++) {
                const base=height*(0.56+layer*0.13);
                c.beginPath(); c.moveTo(0,base);
                c.bezierCurveTo(width*0.17,base-height*0.13,width*0.25,base+height*0.15,width*0.47,base);
                c.bezierCurveTo(width*0.73,base-height*0.2,width*0.85,base-height*0.03,width,base-height*0.1);
                c.lineTo(width,height); c.lineTo(0,height); c.closePath();
                c.fillStyle=["rgba(21,44,68,0.16)","rgba(20,40,61,0.3)","rgba(16,32,52,0.5)"][layer]; c.fill();
            }
        }
    }
    Repeater {
        model: sky.rain || sky.snow ? 42 : 0
        Rectangle {
            required property int index
            x: ((index*137.51)%1000)/1000*sky.width
            width: sky.snow ? 3+(index%3) : 1
            height: sky.snow ? width : 14 + index%15
            radius: width
            color: "#e0f1ff"; opacity: sky.snow ? 0.45 : 0.14
            rotation: sky.snow ? 0 : 16
            YAnimator on y {
                from: -80; to: sky.height+80
                duration: (sky.snow ? 12000 : 1900) + index*39
                running: sky.animate; loops: Animation.Infinite
            }
            Component.onCompleted: y = (index*43)%Math.max(1,sky.height)
        }
    }
}
