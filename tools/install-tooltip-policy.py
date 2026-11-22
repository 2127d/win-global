#!/usr/bin/env python3
"""Install the Sequoia dialog and per-section tooltip policy with atomic backups."""
import hashlib,json,os,shutil,tempfile
from datetime import datetime
from pathlib import Path
from quick_islands import ROOT,QML
from tooltip_styles import desktop,plasma
base=Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local/share'))
updates={}
def add(target,source):updates[target]=source.read_bytes()
plugins=ROOT/'.staging/desktop/usr/lib64/qt6/plugins'
for name in ('sequoiaeleven6.so','sequoiaeleven_glass6.so'):
    add(base/'sequoia-eleven/downloads-style/styles'/name,plugins/'styles'/name)
add(base/'sequoia-eleven/downloads-style/kstyle_config/sequoiaelevenstyleconfig.so',plugins/'kstyle_config/sequoiaelevenstyleconfig.so')
module=ROOT/'.staging/glass-material/libsequoiaglassplugin.so'
for overlay in ('newstuff','quick-islands','motion','settings-hub-preview'):
    qml=base/'sequoia-eleven'/overlay/'qml'
    if not qml.exists():continue
    add(qml/'org/sequoia/glass/libsequoiaglassplugin.so',module)
    updates[qml/'org/sequoia/glass/qmldir']=b'module org.sequoia.glass\noptional plugin sequoiaglassplugin\n'
    if (qml/'org/kde/desktop').exists():
        updates[qml/'org/kde/desktop/ToolTip.qml']=desktop((QML/'org/kde/desktop/ToolTip.qml').read_text()).encode()
    if overlay in ('newstuff','quick-islands'):
        add(qml/'org/kde/plasma/core/DialogBackground.qml',ROOT/'themes/application-style/quick/plasma/DialogBackground.qml')
    if (qml/'org/kde/plasma/components/ToolTip.qml').exists():
        updates[qml/'org/kde/plasma/components/ToolTip.qml']=plasma((QML/'org/kde/plasma/components/ToolTip.qml').read_text()).encode()
desktop_entry=Path('/usr/share/applications/sequoiaelevenstyleconfig.desktop')
if desktop_entry.exists():
    updates[base/'applications/sequoiaelevenstyleconfig.desktop']=desktop_entry.read_text().replace('Exec=kcmshell6 sequoiaelevenstyleconfig','Exec=kcmshell6 kstyle_config/sequoiaelevenstyleconfig').encode()
receipt=base/'sequoia-eleven/theme-installation/manifest.json'
if receipt.is_file():
    state=json.loads(receipt.read_text())
    for path,data in updates.items():
        key=str(path.relative_to(base))
        if key in state['files']:state['files'][key]['installed']='sha256:'+hashlib.sha256(data).hexdigest()
    updates[receipt]=(json.dumps(state,indent=2)+'\n').encode()
backup=base/'sequoia-eleven/backups'/('tooltip-policy-'+datetime.now().strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True)
records=[]
for i,path in enumerate(updates):
    previous=backup/str(i)
    if path.exists():shutil.copy2(path,previous)
    records.append({'path':str(path),'backup':str(previous) if path.exists() else None})
(backup/'files.json').write_text(json.dumps(records,indent=2)+'\n')
for path,data in updates.items():
    path.parent.mkdir(parents=True,exist_ok=True)
    fd,name=tempfile.mkstemp(prefix='.tooltip-policy-',dir=path.parent);os.close(fd)
    pending=Path(name);pending.write_bytes(data);pending.chmod(path.stat().st_mode&0o777 if path.exists() else 0o644);pending.replace(path)
print('Installed Sequoia dialog and tooltip policy. Backup:',backup)
