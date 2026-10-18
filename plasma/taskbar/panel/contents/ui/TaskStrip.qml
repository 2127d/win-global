// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQml.Models
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC3
import org.kde.kirigami as Kirigami
import org.kde.taskmanager as TaskManager
import org.sequoia.taskbar 1.0 as Taskbar
import "Metrics.js" as Metrics

Item {
    id: strip
    objectName: "sequoiaTaskStrip"
    required property bool horizontal
    required property rect screenGeometry
    required property int location
    readonly property var settings: config.values
    readonly property real thickness: horizontal ? height : width
    readonly property real length: horizontal ? width : height
    readonly property int effectiveIconSize: Metrics.iconSize(settings.sizeMode, settings.iconSize, thickness, settings.padding)
    readonly property real taskLength: Math.max(0, length)
    readonly property real cellLength: Metrics.buttonLength(settings.sizeMode, effectiveIconSize, settings.padding, settings.spacing, tasks.count, taskLength)
    readonly property real contentLength: Math.max(0, tasks.count * cellLength + (tasks.count - 1) * settings.spacing)
    readonly property real taskOffset: Metrics.offset(settings.alignment, taskLength, contentLength)
    readonly property int taskCount: tasks.count
    readonly property real naturalLength: Metrics.naturalLength(effectiveIconSize, settings.padding, settings.spacing, tasks.count)
    readonly property real inwardDirection: location === PlasmaCore.Types.TopEdge || location === PlasmaCore.Types.LeftEdge ? 1 : -1
    readonly property bool moving: settings.dockAnimations && Kirigami.Units.longDuration > 1
    readonly property int motionDuration: moving ? Math.round(370 * 100 / settings.dockSpeed) : 0
    readonly property bool reverse: horizontal && Application.layoutDirection === Qt.RightToLeft
    Taskbar.Settings { id: config }
    TaskManager.VirtualDesktopInfo { id: desktops }
    TaskManager.ActivityInfo { id: activities }
    TaskManager.TasksModel {
        id: tasks
        Component.onCompleted: launcherList = Qt.binding(() => strip.settings.launchers)
        screenGeometry: strip.screenGeometry
        activity: activities.currentActivity
        virtualDesktop: desktops.currentDesktop
        filterByActivity: true
        filterByScreen: strip.settings.currentScreenOnly
        filterByCurrentVirtualDesktop: strip.settings.currentDesktopOnly
        sortMode: TaskManager.TasksModel.SortManual
        groupMode: TaskManager.TasksModel.GroupApplications
        groupInline: false
        groupingWindowTasksThreshold: -1
        launchInPlace: true
        separateLaunchers: false
        hideActivatedLaunchers: true
    }
    function pin(url) {
        if (!url) return;
        if (tasks.launcherPosition(url) >= 0) tasks.requestRemoveLauncher(url);
        else tasks.requestAddLauncher(url);
        config.setValue("launchers", tasks.launcherList);
        config.save();
    }
    ListView {
        id: taskView
        objectName: "integratedTasks"
        x: strip.horizontal ? (strip.reverse ? 0 : strip.taskOffset) : 0
        y: strip.horizontal ? 0 : strip.taskOffset
        width: strip.horizontal ? Math.max(0, strip.taskLength - strip.taskOffset) : strip.width
        height: strip.horizontal ? strip.height : Math.max(0, strip.taskLength - strip.taskOffset)
        orientation: strip.horizontal ? ListView.Horizontal : ListView.Vertical
        spacing: strip.settings.spacing
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: tasks
        layoutDirection: Application.layoutDirection
        displaced: Transition {
            NumberAnimation {
                properties: "x,y"
                duration: strip.settings.dockReorderAnimation ? strip.motionDuration : 0
                easing.type: Easing.OutCubic
            }
        }
        WheelHandler {
            enabled: strip.horizontal && taskView.contentWidth > taskView.width
            onWheel: event => {
                const delta = event.angleDelta.x || event.angleDelta.y;
                taskView.contentX = Math.max(taskView.originX, Math.min(taskView.originX + taskView.contentWidth - taskView.width, taskView.contentX - delta));
                event.accepted = true;
            }
        }
        PC3.ScrollBar.horizontal: PC3.ScrollBar { policy: strip.horizontal && taskView.contentWidth > taskView.width ? PC3.ScrollBar.AsNeeded : PC3.ScrollBar.AlwaysOff }
        PC3.ScrollBar.vertical: PC3.ScrollBar { policy: !strip.horizontal && taskView.contentHeight > taskView.height ? PC3.ScrollBar.AsNeeded : PC3.ScrollBar.AlwaysOff }
        delegate: PC3.ToolButton {
            id: task
            objectName: "integratedTaskButton"
            required property int index
            required property var model
            readonly property bool isRunning: !!(model.IsWindow || model.IsGroupParent)
            readonly property bool isActiveTask: !!model.IsActive && !model.IsMinimized
            readonly property bool isMinimized: !!model.IsMinimized
            readonly property bool isGroup: !!model.IsGroupParent
            width: strip.horizontal ? strip.cellLength : strip.width
            height: strip.horizontal ? strip.height : strip.cellLength
            text: model.display || model.AppName || ""
            Accessible.name: text
            Accessible.description: model.IsActive ? i18n("Active application") : model.IsWindow || model.IsGroupParent ? i18n("Running application") : i18n("Pinned application")
            contentItem: Item {
                Taskbar.MotionIcon {
                    id: taskIcon
                    objectName: "taskIcon"
                    anchors.centerIn: parent
                    width: strip.effectiveIconSize
                    height: strip.effectiveIconSize
                    source: task.model.decoration || "application-x-executable"
                    settings: strip.settings
                    hovered: task.hovered || taskTip.containsMouse
                    pressed: task.down
                    launching: !!task.model.IsStartup || launchCue.running
                    demandingAttention: !!task.model.IsDemandingAttention
                    horizontal: strip.horizontal
                    inwardDirection: strip.inwardDirection
                    viewportWidth: task.width
                    viewportHeight: task.height
                }
            }
            // Also give immediate feedback for launchers whose startup role is
            // too brief to render. Combining the cues avoids a second bounce.
            Timer { id: launchCue; interval: Math.max(1, strip.motionDuration); }
            highlighted: isActiveTask || !!model.IsDemandingAttention
            onClicked: {
                if (!isRunning) launchCue.restart();
                const idx = tasks.makeModelIndex(index);
                if (model.IsGroupParent) {
                    groupTasks.rootIndex = idx;
                    groupPopup.visualParent = task;
                    groupPopup.visible = true;
                } else if (isActiveTask) tasks.requestToggleMinimized(idx);
                else tasks.requestActivate(idx);
            }
            PlasmaCore.ToolTipArea {
                id: taskTip
                anchors.fill: parent
                mainText: task.text
                location: strip.location
                active: !groupPopup.visible && !contextMenu.visible
                interactive: false
            }
            Rectangle {
                objectName: "runningIndicator"
                visible: strip.settings.showIndicators && task.isRunning
                width: strip.horizontal ? (task.isActiveTask ? 16 : 6) : 3
                height: strip.horizontal ? 3 : (task.isActiveTask ? 16 : 6)
                x: strip.horizontal ? (parent.width - width) / 2 : parent.width - width - 1
                y: strip.horizontal ? parent.height - height - 1 : (parent.height - height) / 2
                radius: 2
                color: task.isActiveTask || task.model.IsDemandingAttention ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
                opacity: (task.isActiveTask ? 0.95 : 0.55) * taskIcon.attentionPulse
            }
            TapHandler {
                acceptedButtons: Qt.MiddleButton
                onTapped: { launchCue.restart(); tasks.requestNewInstance(tasks.makeModelIndex(task.index)); }
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: {
                    contextMenu.taskIndex = task.index;
                    contextMenu.launcherUrl = task.model.LauncherUrlWithoutIcon || "";
                    contextMenu.running = !!(task.model.IsWindow || task.model.IsGroupParent);
                    contextMenu.visualParent = task;
                    contextMenu.visible = true;
                }
            }
        }
    }
    PlasmaCore.Dialog {
        id: contextMenu
        property int taskIndex: -1
        property string launcherUrl
        property bool running: false
        location: strip.location
        type: PlasmaCore.Dialog.PopupMenu
        hideOnWindowDeactivate: true
        visible: false
        mainItem: Column {
            width: Kirigami.Units.gridUnit * 14
            PC3.Button {
                width: parent.width
                text: tasks.launcherPosition(contextMenu.launcherUrl) >= 0 ? i18n("Unpin application") : i18n("Pin application")
                enabled: contextMenu.launcherUrl.length > 0
                onClicked: { strip.pin(contextMenu.launcherUrl); contextMenu.visible = false; }
            }
            PC3.Button {
                width: parent.width
                text: i18n("New window")
                onClicked: { tasks.requestNewInstance(tasks.makeModelIndex(contextMenu.taskIndex)); contextMenu.visible = false; }
            }
            PC3.Button {
                width: parent.width
                visible: contextMenu.running
                text: i18n("Close")
                onClicked: { tasks.requestClose(tasks.makeModelIndex(contextMenu.taskIndex)); contextMenu.visible = false; }
            }
            PC3.Button {
                width: parent.width
                text: i18n("Taskbar Settings…")
                onClicked: { config.openSettings(); contextMenu.visible = false; }
            }
        }
    }
    PlasmaCore.Dialog {
        id: groupPopup
        location: strip.location
        type: PlasmaCore.Dialog.PopupMenu
        hideOnWindowDeactivate: true
        visible: false
        mainItem: ListView {
            width: Kirigami.Units.gridUnit * 20
            height: Math.min(contentHeight, Kirigami.Units.gridUnit * 20)
            clip: true
            model: DelegateModel {
                id: groupTasks
                model: tasks
                delegate: PC3.ItemDelegate {
                    id: groupedTask
                    required property int index
                    required property var model
                    width: ListView.view.width
                    text: model.display || ""
                    contentItem: RowLayout {
                        Kirigami.Icon { source: groupedTask.model.decoration; Layout.preferredWidth: 24; Layout.preferredHeight: 24 }
                        PC3.Label { text: groupedTask.text; Layout.fillWidth: true; elide: Text.ElideRight }
                    }
                    onClicked: { tasks.requestActivate(groupTasks.modelIndex(index)); groupPopup.visible = false; }
                }
            }
        }
    }
}
