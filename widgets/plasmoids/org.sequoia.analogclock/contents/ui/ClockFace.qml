/*
    SPDX-FileCopyrightText: 2012 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013-2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
import QtQuick
import org.kde.ksvg as KSvg
import org.kde.kirigami as Kirigami

Item {
    id: root
    objectName: "sequoiaAnalogFace"
    property bool rounded: true
    property int hours: 10
    property int minutes: 10
    property int seconds: 30
    property bool showSecondsHand: false
    KSvg.Svg {
        id: clockSvg

        property double naturalHorizontalHandShadowOffset: estimateHorizontalHandShadowOffset()
        property double naturalVerticalHandShadowOffset: estimateVerticalHandShadowOffset()

        imagePath: root.rounded ? Qt.resolvedUrl("../images/clock.svg") : "widgets/clock"
        function estimateHorizontalHandShadowOffset() {
            var id = "hint-hands-shadow-offset-to-west";
            if (hasElement(id)) {
                return -elementSize(id).width;
            }
            id = "hint-hands-shadows-offset-to-east";
            if (hasElement(id)) {
                return elementSize(id).width;
            }
            return 0;
        }
        function estimateVerticalHandShadowOffset() {
            var id = "hint-hands-shadow-offset-to-north";
            if (hasElement(id)) {
                return -elementSize(id).height;
            }
            id = "hint-hands-shadow-offset-to-south";
            if (hasElement(id)) {
                return elementSize(id).height;
            }
            return 0;
        }

        onRepaintNeeded: {
            naturalHorizontalHandShadowOffset = estimateHorizontalHandShadowOffset();
            naturalVerticalHandShadowOffset = estimateVerticalHandShadowOffset();
        }
    }

    Item {
        id: clock

        anchors.fill: parent

        readonly property double svgScale: face.width / face.naturalSize.width
        readonly property double horizontalShadowOffset:
            Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalHorizontalHandShadowOffset * svgScale) % 2
        readonly property double verticalShadowOffset:
            Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) + Math.round(clockSvg.naturalVerticalHandShadowOffset * svgScale) % 2

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
            radius: Math.min(24, width / 2)
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2)
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            visible: root.rounded
        }
        KSvg.SvgItem {
            id: face
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: Math.min(parent.width, parent.height)
            svg: clockSvg
            elementId: "ClockFace"
        }

        Hand {
            svg: clockSvg
            elementId: "HourHandShadow"
            rotationCenterHintId: "hint-hourhandshadow-rotation-center-offset"
            horizontalRotationOffset: clock.horizontalShadowOffset
            verticalRotationOffset: clock.verticalShadowOffset
            handRotation: 180 + root.hours * 30 + (root.minutes/2)
            svgScale: clock.svgScale
        }
        Hand {
            svg: clockSvg
            elementId: "HourHand"
            rotationCenterHintId: "hint-hourhand-rotation-center-offset"
            handRotation: 180 + root.hours * 30 + (root.minutes/2)
            svgScale: clock.svgScale
        }

        Hand {
            svg: clockSvg
            elementId: "MinuteHandShadow"
            rotationCenterHintId: "hint-minutehandshadow-rotation-center-offset"
            horizontalRotationOffset: clock.horizontalShadowOffset
            verticalRotationOffset: clock.verticalShadowOffset
            handRotation: 180 + root.minutes * 6
            svgScale: clock.svgScale
        }
        Hand {
            svg: clockSvg
            elementId: "MinuteHand"
            rotationCenterHintId: "hint-minutehand-rotation-center-offset"
            handRotation: 180 + root.minutes * 6
            svgScale: clock.svgScale
        }

        Hand {
            svg: clockSvg
            visible: root.showSecondsHand
            elementId: "SecondHandShadow"
            rotationCenterHintId: "hint-secondhandshadow-rotation-center-offset"
            horizontalRotationOffset: clock.horizontalShadowOffset
            verticalRotationOffset: clock.verticalShadowOffset
            handRotation: 180 + root.seconds * 6
            svgScale: clock.svgScale
        }
        Hand {
            svg: clockSvg
            visible: root.showSecondsHand
            elementId: "SecondHand"
            rotationCenterHintId: "hint-secondhand-rotation-center-offset"
            handRotation: 180 + root.seconds * 6
            svgScale: clock.svgScale
        }

        KSvg.SvgItem {
            id: center
            anchors.centerIn: clock
            width: naturalSize.width * clock.svgScale
            height: naturalSize.height * clock.svgScale
            svg: clockSvg
            elementId: "HandCenterScrew"
            z: 1000
        }

        KSvg.SvgItem {
            anchors.fill: face
            width: naturalSize.width * clock.svgScale
            height: naturalSize.height * clock.svgScale
            svg: clockSvg
            elementId: "Glass"
        }
    }

}
