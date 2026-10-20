/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Templates as T

import org.kde.ksvg as KSvg
import org.kde.plasma.plasmoid

import plasma.applet.org.kde.plasma.taskmanager as TaskManagerApplet

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    hoverEnabled: false
    padding: 0

    from: 0
    to: 100
    value: task.smartLauncherItem.progress

    contentItem: Item {
        clip: true

        Item {
            id: progressClip

            // Reveal the full-size themed frame. Resizing its nine-slice artwork
            // below the combined border widths makes the end caps overlap.
            width: parent.width * control.position
            height: parent.height
            x: control.mirrored ? parent.width - width : 0
            clip: true

            KSvg.FrameSvgItem {
                id: progressFrame

                x: control.mirrored ? progressClip.width - width : 0
                width: control.availableWidth
                height: parent.height

                imagePath: "widgets/tasks"
                prefix: TaskManagerApplet.TaskTools.taskPrefix("progress", Plasmoid.location).concat(TaskManagerApplet.TaskTools.taskPrefix("hover", Plasmoid.location))
            }
        }
    }

    background: null
}
