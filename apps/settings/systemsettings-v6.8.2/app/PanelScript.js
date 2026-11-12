// SPDX-License-Identifier: GPL-2.0-or-later
// Runs in Plasma's scripting engine. Never writes configuration files directly.
function sequoiaPanelSettings(command) {
    var error = "";
    var list = panels();
    try {
        if (command.action) {
            var p = list.find(function(panel) { return panel.id === command.id; });
            if (!p) throw new Error(i18n("The selected panel was removed. Choose another panel."));
            if (p.locked || immutability() !== "mutable") throw new Error(i18n("Unlock widgets before changing this panel."));
            if (command.action === "edit") p.showConfigurationInterface();
            else if (command.action === "set") {
                var key = command.key, value = command.value;
                var geometry = screenGeometry(p.screen);
                var vertical = p.location === "left" || p.location === "right";
                var side = vertical ? geometry.height : geometry.width;
                if (key === "floatingMode") {
                    p.floating = value === "panel";
                    p.floatingApplets = value !== "disabled";
                } else if (key === "screen") {
                    if (value < 0 || value >= __AppInterface.screenCount) throw new Error(i18n("That screen is no longer connected."));
                    p.screen = value;
                } else if (key === "height") p.height = Math.max(20, Math.min(Math.floor((vertical ? geometry.width : geometry.height) / 2), value));
                else if (key === "offset") p.offset = Math.max(0, Math.min(Math.max(0, side - p.minimumLength), value));
                else if (key === "minimumLength") {
                    if (p.lengthMode !== "custom") throw new Error(i18n("Select Custom size first."));
                    value = Math.max(10, Math.min(side - p.offset, value));
                    if (value > p.maximumLength) p.maximumLength = value;
                    p.minimumLength = value;
                } else if (key === "maximumLength") {
                    if (p.lengthMode !== "custom") throw new Error(i18n("Select Custom size first."));
                    value = Math.max(10, Math.min(side - p.offset, value));
                    if (value < p.minimumLength) p.minimumLength = value;
                    p.maximumLength = value;
                } else if (["location", "alignment", "lengthMode", "hiding", "opacity"].indexOf(key) !== -1) p[key] = value;
            }
        }
    } catch (exception) { error = exception.message; }
    var screens = [];
    for (var index = 0; index < __AppInterface.screenCount; ++index) {
        var geometry = screenGeometry(index);
        screens.push({name:i18n("Screen %1 · %2 × %3", index + 1, geometry.width, geometry.height), value:index});
    }
    var views = new ConfigFile("plasmashellrc", "PlasmaViews");
    var snapshot = panels().map(function(p) {
        var view = new ConfigFile(views, "Panel " + p.id);
        // Plasma 6.8's hiding getter omits WindowsGoBelow. Its live config group
        // preserves that fourth enum without treating it as Always visible.
        var visibility = Number(view.readEntry("panelVisibility", 0));
        var geometry = screenGeometry(p.screen);
        var vertical = p.location === "left" || p.location === "right";
        var edges = {top:i18n("Top"), bottom:i18n("Bottom"), left:i18n("Left"), right:i18n("Right")};
        return {id:p.id, name:i18n("%1 · Screen %2 · #%3", edges[p.location] || p.location, p.screen + 1, p.id),
            location:p.location, screen:p.screen, height:p.height, alignment:p.alignment, lengthMode:p.lengthMode,
            hiding:visibility === 3 ? "windowsgobelow" : p.hiding, opacity:p.opacity,
            floatingMode:p.floating ? "panel" : p.floatingApplets ? "applets" : "disabled",
            minimumLength:p.minimumLength, maximumLength:p.maximumLength, offset:p.offset,
            side:vertical ? geometry.height : geometry.width, maxThickness:Math.floor((vertical ? geometry.width : geometry.height) / 2),
            locked:p.locked || immutability() !== "mutable"};
    });
    print(JSON.stringify({panels:snapshot, screens:screens, error:error}));
}
