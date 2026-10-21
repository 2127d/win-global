// SPDX-License-Identifier: MIT
pragma Singleton
import QtQuick
import org.kde.plasma.private.shell
import org.kde.kwindowsystem
import "WidgetCatalog.js" as Catalog

QtObject {
    id: session
    readonly property int cell: 96
    property var targets: []
    property string phase: "catalog"
    property string plugin: ""
    property string title: ""
    property string icon: ""
    property var destination: null
    property int presetIndex: 0
    property string error: ""
    readonly property var definition: Catalog.widgets[plugin] || {
        panel: true, presets: [{name: "Default", columns: 2, rows: 2}], preview: ""
    }
    readonly property var preset: definition.presets[presetIndex] || definition.presets[0]
    readonly property bool hidden: phase === "placement" || phase === "drag"
    signal finished()
    property QtObject creator: WidgetExplorer {}
    function addApplet(target, pluginName) {
        const previous = target.plasmoid.applets.map(a => a.id);
        creator.containment = target.plasmoid;
        creator.addApplet(pluginName);
        const added = target.plasmoid.applets.find(a => previous.indexOf(a.id) < 0);
        creator.containment = null;
        return added || null;
    }

    function registerTarget(target) {
        if (targets.indexOf(target) < 0) targets = targets.concat([target]);
    }
    function unregisterTarget(target) {
        targets = targets.filter(t => t !== target);
        if (destination === target) cancel();
    }
    function supports(pluginName) { return !!Catalog.widgets[pluginName]; }
    function begin(pluginName, name, decoration, dragging) {
        plugin = pluginName;
        title = name;
        icon = decoration;
        destination = null;
        presetIndex = 0;
        error = "";
        phase = dragging ? "drag" : "destination";
    }
    function choose(target) {
        if (!target.ready) { error = qsTr("This destination is no longer available."); return; }
        destination = target;
        error = "";
        if (target.kind === "panel") {
            if (!definition.panel) { error = qsTr("This widget needs a desktop."); return; }
            if (target.addPanelWidget(plugin)) complete();
        } else {
            phase = "preset";
        }
    }
    function place() {
        if (!destination || !destination.ready) { cancel(); return; }
        KWindowSystem.showingDesktop = true;
        phase = "placement";
        destination.activatePlacement();
    }
    function complete() {
        phase = "catalog";
        destination = null;
        error = "";
        finished();
    }
    function cancel() { complete(); }
}
