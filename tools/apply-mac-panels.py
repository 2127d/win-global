#!/usr/bin/env python3
"""Refine existing top/bottom panels through the live Plasma scripting API."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def script(downloads):
    return r'''
function settings(widget, group, values) {
    widget.currentConfigGroup = group;
    for (var key in values) widget.writeConfig(key, values[key]);
    widget.reloadConfig();
}
var all = panels();
var dock = all.find(p => p.location === "bottom" && p.widgets().some(w => w.type === "org.kde.plasma.icontasks"));
var bar = all.find(p => p.location === "top" && p.widgets().some(w => w.type === "org.kde.plasma.appmenu"));
if (!dock || !bar) throw new Error("Expected existing dock and global-menu panel; no changes made.");
dock.height = 68;
dock.floating = true;
dock.alignment = "center";
dock.lengthMode = "fit";
var tasks = dock.widgets().find(w => w.type === "org.kde.plasma.icontasks");
settings(tasks, ["General"], {iconSpacing: 1, maxStripes: 1, taskHoverEffect: true});
var downloads = dock.widgets().find(w => w.type === "org.sequoia.downloads") || dock.addWidget("org.sequoia.downloads");
settings(downloads, ["General"], {folderUrl: DOWNLOADS});
dock.widgets().filter(w => {
    if (w.type !== "org.kde.plasma.quicklaunch") return false;
    w.currentConfigGroup = ["General"];
    var urls = w.readConfig("launcherUrls", []);
    return urls.length === 1 && String(urls[0]) === DOWNLOADS;
}).forEach(w => w.remove());
var trash = dock.widgets().find(w => w.type === "org.kde.plasma.trash") || dock.addWidget("org.kde.plasma.trash");
var separator = dock.widgets().find(w => w.type === "org.sequoia.dockseparator") || dock.addWidget("org.sequoia.dockseparator");
var dockOrder = dock.widgets().filter(w => !["org.sequoia.downloads", "org.kde.plasma.trash", "org.sequoia.dockseparator", "org.kde.plasma.icontasks"].includes(w.type));
// Keep launcher widgets to the left and the existing task order untouched.
dockOrder = dockOrder.concat([tasks, separator, downloads, trash]);
dockOrder.forEach((widget, index) => { widget.index = index; });

bar.height = 32;
bar.floating = false;
var widgets = bar.widgets();
var title = widgets.find(w => w.type === "org.kde.windowtitle.Fork");
if (title) settings(title, ["General"], {style: 0, boldFont: true, showIcon: false, lengthFirstMargin: 6, lengthLastMargin: 6, lengthMarginsLock: true});
var clock = widgets.find(w => w.type === "org.kde.plasma.digitalclock");
if (clock) settings(clock, ["Appearance"], {autoFontAndSize: false, fontFamily: "Adwaita Sans", fontSize: 10, fontWeight: 400, fontStyleName: "Regular", boldText: false});
var tray = widgets.find(w => w.type === "org.kde.plasma.systemtray");
if (tray) {
    tray.currentConfigGroup = ["General"];
    var secondary = ["org.kde.plasma.volume", "org.kde.plasma.bluetooth", "org.kde.plasma.brightness", "org.kde.plasma.weather", "org.kde.plasma.clipboard", "org.kde.plasma.devicenotifier", "org.kde.plasma.mediacontroller", "org.kde.merkuro.contact.applet", "org.kde.kdeconnect", "org.kde.kscreen", "org.kde.plasma.printmanager", "org.kde.plasma.vault"];
    var hidden = tray.readConfig("hiddenItems", []);
    var shown = tray.readConfig("shownItems", []);
    tray.writeConfig("hiddenItems", Array.from(new Set(hidden.concat(secondary))));
    tray.writeConfig("shownItems", shown.filter(id => !secondary.includes(id)));
    tray.writeConfig("showAllItems", false);
    tray.writeConfig("iconSpacing", 2);
    tray.reloadConfig();
}
var spacer = widgets.find(w => w.type === "org.kde.plasma.panelspacer");
// Margin separators at every boundary and duplicate expanding spacers add noise.
widgets.filter(w => w.type === "org.kde.plasma.marginsseparator" || (w.type === "org.kde.plasma.panelspacer" && w.id !== spacer.id)).forEach(w => w.remove());
widgets = bar.widgets();
var left = widgets.filter(w => w.type === "org.kpple.kppleMenu");
if (title) left.push(title);
left.push(widgets.find(w => w.type === "org.kde.plasma.appmenu"));
if (spacer) left.push(spacer);
var ids = left.map(w => w.id);
var rest = widgets.filter(w => !ids.includes(w.id) && (!clock || w.id !== clock.id));
var order = left.concat(rest);
if (clock) order.push(clock);
order.forEach((widget, index) => { widget.index = index; });
print("Dock and menu bar refinements applied.");
'''.replace('DOWNLOADS', json.dumps(downloads))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    downloads = subprocess.check_output(['xdg-user-dir', 'DOWNLOAD'], text=True).strip()
    code = script(Path(downloads).as_uri())
    if args.dry_run:
        print(code)
        return
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    backup = data / 'sequoia-eleven/backups' / ('before-mac-panels-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    for name in ('plasma-org.kde.plasma.desktop-appletsrc', 'plasmashellrc'):
        shutil.copy2(config / name, backup / name)
    for plugin in ('org.sequoia.dockseparator', 'org.sequoia.downloads'):
        target = data / 'plasma/plasmoids' / plugin
        if target.exists():
            shutil.copytree(target, backup / plugin)
        shutil.copytree(ROOT / 'widgets/plasmoids' / plugin, target, dirs_exist_ok=True)
    print('Previous panel configuration:', backup, flush=True)
    result = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript', code], check=True, text=True, capture_output=True, timeout=30)
    print(result.stdout)
    if 'Dock and menu bar refinements applied.' not in result.stdout:
        raise RuntimeError('Plasma did not finish the panel update: ' + result.stderr)


if __name__ == '__main__':
    main()
