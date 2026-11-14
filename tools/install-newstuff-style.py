#!/usr/bin/env python3
"""Install the shared Get New presentation and solid-toolbar native styles per user."""
import argparse,base64,hashlib,json,os,shutil,tempfile
from datetime import datetime
from pathlib import Path
from quick_islands import ROOT,QML

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--stage',type=Path,help='Prepare only the minimal shared QML overlay')
a=p.parse_args()
source=ROOT/'.staging/quick-islands'
manifest=json.loads((source/'manifest.json').read_text())
shared_sources={k:v for k,v in manifest['sources'].items()
                if any(k.startswith(str(QML/module)+'/') for module in ('org/kde/newstuff','org/kde/desktop'))}
assert shared_sources,'Build the Quick overlay first'
for path,digest in shared_sources.items():
    if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('KDE changed; rebuild the Quick overlay first.')

def stage(destination):
    destination.mkdir(parents=True,exist_ok=True)
    for relative in ('org/kde/newstuff','org/sequoia/components'):
        shutil.copytree(source/relative,destination/relative,symlinks=True,dirs_exist_ok=True)
    # Other applications get stock desktop controls with only the toolbar
    # surface changed. Do not inject unrelated Kirigami or Plasma overrides.
    desktop=destination/'org/kde/desktop'
    shutil.copytree(QML/'org/kde/desktop',desktop,dirs_exist_ok=True)
    for path in desktop.rglob('*'):
        if path.is_file() and '.so' in path.name:
            path.unlink();path.symlink_to(QML/'org/kde/desktop'/path.relative_to(desktop))
        elif path.name=='qmldir':
            path.write_text(''.join(line for line in path.read_text().splitlines(True) if not line.startswith('prefer ')))
    shutil.copy2(source/'org/kde/desktop/ToolBar.qml',desktop/'ToolBar.qml')
    island=destination/'org/sequoia/eleven';island.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'themes/application-style/quick/Island.qml',island/'Island.qml')
    (island/'qmldir').write_text('module org.sequoia.eleven\nIsland 1.0 Island.qml\n')
    (destination/'manifest.json').write_text(json.dumps({'sources':shared_sources},indent=2)+'\n')
if a.stage:
    dest=a.stage/'qml'
    if dest.exists() and not (dest/'manifest.json').is_file():
        raise RuntimeError('Refusing to replace an unmanaged stage: '+str(dest))
    dest.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.newstuff-stage-',dir=dest.parent) as temporary:
        prepared=Path(temporary)/'ready';stage(prepared)
        if dest.exists():dest.rename(Path(temporary)/'previous')
        prepared.rename(dest)
    print(a.stage);raise SystemExit(0)

data=Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local/share'))
quick=data/'sequoia-eleven/quick-islands/qml'
if not (quick/'manifest.json').is_file():raise RuntimeError('Install the shared Quick overlay first.')
style_dir=data/'sequoia-eleven/downloads-style/styles'
if not (style_dir/'sequoiaeleven6.so').exists():raise RuntimeError('Expected the existing per-user native style installation.')
# Validate active glass edits before touching files; refresh_quick retains their
# restoration baselines and upgrades the toolbar while preserving glass popups.
from app_glass import STATE
if STATE.exists():
    for entry in json.loads(STATE.read_text()).get('files',[]):
        if Path(entry['path']).read_bytes()!=base64.b64decode(entry['after']):
            raise RuntimeError('Preserve externally edited glass overlay: '+entry['path'])
backup=data/'sequoia-eleven/backups'/('before-shared-newstuff-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True)
target=data/'sequoia-eleven/newstuff/qml'
paths=[target,quick/'org/kde/newstuff',quick/'org/sequoia/components',quick/'manifest.json',
       quick/'org/kde/desktop/ToolBar.qml',quick/'org/kde/desktop/Popup.qml',STATE,
       style_dir/'sequoiaeleven6.so',style_dir/'sequoiaeleven_glass6.so']
records=[]
for i,path in enumerate(paths):
    record={'path':str(path),'backup':str(backup/str(i)) if path.exists() else None};records.append(record)
    if path.is_dir():shutil.copytree(path,backup/str(i),symlinks=True)
    elif path.exists():shutil.copy2(path,backup/str(i))
(backup/'files.json').write_text(json.dumps(records,indent=2)+'\n')
try:
    # Prepare directory swaps before publishing, preserving every unrelated QML module.
    for dest in (target,quick/'org/kde/newstuff',quick/'org/sequoia/components'):
        dest.parent.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='.newstuff-',dir=dest.parent) as temporary:
            prepared=Path(temporary)/'ready'
            if dest==target:stage(prepared)
            else:shutil.copytree(source/('org/kde/newstuff' if dest.name=='newstuff' else 'org/sequoia/components'),prepared,symlinks=True)
            if dest.exists():dest.rename(Path(temporary)/'previous')
            prepared.rename(dest)
    current=json.loads((quick/'manifest.json').read_text());current['sources'].update(shared_sources)
    pending=quick/'manifest.json.new';pending.write_text(json.dumps(current,indent=2)+'\n');pending.replace(quick/'manifest.json')
    from glass_surfaces import refresh_quick
    refresh_quick()
    for name in ('sequoiaeleven6.so','sequoiaeleven_glass6.so'):
        library=ROOT/'.staging/desktop/usr/lib64/qt6/plugins/styles'/name
        pending=style_dir/(name+'.new');shutil.copy2(library,pending);pending.replace(style_dir/name)
except BaseException:
    for r in reversed(records):
        path=Path(r['path'])
        if path.is_dir():shutil.rmtree(path)
        else:path.unlink(missing_ok=True)
        if r['backup']:
            original=Path(r['backup']);path.parent.mkdir(parents=True,exist_ok=True)
            if original.is_dir():shutil.copytree(original,path,symlinks=True)
            else:shutil.copy2(original,path)
    raise
print('Installed shared Get New browser and solid native/Quick toolbars. Reopen applications to load them.')
print('Backup:',backup)
