#!/usr/bin/env python3
"""Install Folder Stack and replace existing Downloads Stack instances in place."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.sequoia.folderstack'
LEGACY = 'org.sequoia.downloads'


def evaluate(code):
    result = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                             'org.kde.PlasmaShell.evaluateScript', code],
                            capture_output=True, text=True, timeout=30)
    if result.returncode: raise RuntimeError(result.stderr or result.stdout)
    return result.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, help='Copy package only into a temporary data root')
    args = parser.parse_args()
    source = ROOT / 'widgets/plasmoids' / PLUGIN
    if args.stage:
        shutil.copytree(source, args.stage / 'plasma/plasmoids' / PLUGIN, dirs_exist_ok=True)
        return
    data = Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share')
    config = Path(os.environ.get('XDG_CONFIG_HOME') or Path.home() / '.config')
    target = data / 'plasma/plasmoids' / PLUGIN
    backup = data / 'sequoia-eleven/backups' / ('before-folder-stack-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    shutil.copy2(config / 'plasma-org.kde.plasma.desktop-appletsrc', backup)
    if target.exists(): shutil.copytree(target, backup / PLUGIN)
    print('Backup:', backup, flush=True)
    subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet', '--packageroot', str(target.parent),
                    '--upgrade' if target.exists() else '--install', str(source)], check=True)
    # Discover all live instances, including secondary panels, without changing other stacks.
    old = json.loads(evaluate('''
print(JSON.stringify(panels().concat(desktops()).map(p=>p.widgets().filter(w=>w.type==="org.sequoia.downloads").map(w=>{
    w.currentConfigGroup=["General"];
    return {panel:p.id,id:w.id,index:w.index,folder:w.readConfig("folderUrl","")};
})).reduce((all, rows)=>all.concat(rows),[])));'''))
    (backup / 'migration.json').write_text(json.dumps(old, indent=2) + '\n')
    for item in old:
        # Configure and verify the replacement before removing the old instance.
        base = 'var p=panelById(PANEL)||desktopById(PANEL);'.replace('PANEL', str(item['panel']))
        created = json.loads(evaluate(base + 'var w=p.addWidget("' + PLUGIN + '"); print(JSON.stringify({id:w.id,type:w.type}));'))
        if created.get('type') != PLUGIN: raise RuntimeError('Plasma could not create Folder Stack; original retained')
        time.sleep(0.3)
        code = base + '''
var w=p.widgetById(@NEW@), old=p.widgetById(@OLD@);
if (!w || !old) throw new Error("Widget missing; original retained");
w.currentConfigGroup=["General"];
w.writeConfig("folder",@FOLDER@);w.writeConfig("viewMode",0);w.reloadConfig();
if (String(w.readConfig("folder",""))!==@FOLDER@) throw new Error("Folder setting failed; original retained");
w.index=@INDEX@;old.remove();w.index=@INDEX@;
print("migrated");'''
        for key, val in [('NEW',created['id']),('OLD',item['id']),('FOLDER',str(item['folder'])),('INDEX',item['index'])]:
            code = code.replace("@" + key + "@", json.dumps(val))
        try:
            result = evaluate(code)
            if result != 'migrated': raise RuntimeError(result)
        except Exception:
            evaluate(base + f'var old=p.widgetById({item["id"]}), w=p.widgetById({created["id"]}); if(old && w) w.remove();')
            raise
    remaining = evaluate('print(panels().concat(desktops()).map(p=>p.widgets()).reduce((all, rows)=>all.concat(rows),[]).filter(w=>w.type==="org.sequoia.downloads").length);')
    if remaining != '0': raise RuntimeError('Some old widgets remain: ' + remaining)
    legacy_package = data / 'plasma/plasmoids' / LEGACY
    if legacy_package.exists():
        shutil.copytree(legacy_package, backup / LEGACY)
        subprocess.run(['kpackagetool6', '--type', 'Plasma/Applet', '--packageroot', str(legacy_package.parent),
                        '--remove', LEGACY], check=True)
    print(f'Installed Folder Stack; replaced {len(old)} Downloads Stack instance(s), retired the old package.')


if __name__ == '__main__':
    main()
