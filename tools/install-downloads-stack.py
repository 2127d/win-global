#!/usr/bin/env python3
"""Replace only the dock's Downloads shortcut with the Downloads fan widget."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
ROOT=Path(__file__).resolve().parents[1]
PLUGIN='org.sequoia.downloads'

def evaluate(code):
    result=subprocess.run(['qdbus-qt6','org.kde.plasmashell','/PlasmaShell','org.kde.PlasmaShell.evaluateScript',code],capture_output=True,text=True,timeout=30)
    if result.returncode: raise RuntimeError(result.stderr or result.stdout)
    return result.stdout

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,help='Install the package into a temporary data directory only')
    args=parser.parse_args()
    source=ROOT/'plasma/plasmoids'/PLUGIN
    if args.stage:
        shutil.copytree(source,args.stage/'plasma/plasmoids'/PLUGIN,dirs_exist_ok=True)
        return
    style=ROOT/'build/desktop/bin/sequoiaeleven6.so'
    if not style.exists():
        raise RuntimeError('Build the Downloads blur opt-out first: tools/build-desktop.sh')
    data=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share')
    config=Path(os.environ.get('XDG_CONFIG_HOME') or Path.home()/'.config')
    target=data/'plasma/plasmoids'/PLUGIN
    backup=data/'sequoia-eleven/backups'/('before-downloads-fan-'+datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    backup.mkdir(parents=True)
    shutil.copy2(config/'plasma-org.kde.plasma.desktop-appletsrc',backup)
    if target.exists(): shutil.copytree(target,backup/PLUGIN)
    # Scope the style override to Plasma; Qt still searches its normal plugin
    # directories. Replace the loaded library atomically, never in place.
    plugins=data/'sequoia-eleven/downloads-style'
    library=plugins/'styles/sequoiaeleven6.so'
    service=config/'systemd/user/plasma-plasmashell.service.d/60-sequoia-downloads-style.conf'
    for path in (library,service):
        if path.exists(): shutil.copy2(path,backup/path.name)
    library.parent.mkdir(parents=True,exist_ok=True)
    staged=library.with_suffix('.so.new')
    shutil.copy2(style,staged)
    staged.replace(library)
    service.parent.mkdir(parents=True,exist_ok=True)
    service.write_text('# Downloads fan: allow this transparent surface to opt out of blur.\n[Service]\nEnvironment="QT_PLUGIN_PATH='+str(plugins)+'"\n')
    subprocess.run(['systemctl','--user','daemon-reload'],check=True)
    shutil.copytree(source,target,dirs_exist_ok=True)
    downloads=Path(subprocess.check_output(['xdg-user-dir','DOWNLOAD'],text=True).strip()).as_uri()
    # Create first, then configure after Plasma has initialized the applet.
    setup='''
var dock=panels().find(p=>p.location==="bottom" && p.widgets().some(w=>w.type==="org.kde.plasma.icontasks"));
if (!dock) throw new Error("No existing bottom dock found");
var stack=dock.widgets().find(w=>w.type==="org.sequoia.downloads") || dock.addWidget("org.sequoia.downloads");
print(JSON.stringify({panel:dock.id,stack:stack.id}));
'''
    ids=json.loads(evaluate(setup));time.sleep(0.3)
    apply='''
var dock=panelById(PANEL), stack=dock.widgetById(STACK);
stack.currentConfigGroup=["General"];stack.writeConfig("folderUrl",URL);stack.reloadConfig();
var legacy=dock.widgets().find(w=>{
    if(w.type!=="org.kde.plasma.quicklaunch") return false;
    w.currentConfigGroup=["General"];
    var urls=w.readConfig("launcherUrls",[]);
    return urls.length===1 && String(urls[0])===URL;
});
if(legacy){stack.index=legacy.index;legacy.remove();}
else {
    var trash=dock.widgets().find(w=>w.type==="org.kde.plasma.trash");
    if(trash && stack.index>trash.index) stack.index=trash.index;
}
print("Downloads fan installed.");
'''.replace('PANEL',str(ids['panel'])).replace('STACK',str(ids['stack'])).replace('URL',json.dumps(downloads))
    result=evaluate(apply)
    if 'Downloads fan installed.' not in result: raise RuntimeError(result)
    print(result.strip());print('Backup:',backup)

if __name__=='__main__':main()
