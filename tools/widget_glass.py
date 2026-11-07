"""Reversible desktop-widget glass overlay, leaving Plasma's system QML intact."""
import base64
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
DATA=Path.home()/'.local/share'
CONFIG=Path.home()/'.config'
STATE=Path.home()/'.local/state/sequoia-eleven/widget-glass-trial.json'
OVERLAY=DATA/'sequoia-eleven/widget-glass/qml'
DROPIN=CONFIG/'systemd/user/plasma-plasmashell.service.d/80-sequoia-widget-glass.conf'
SOURCE=Path('/usr/lib64/qt6/qml/org/kde/plasma/private/containmentlayoutmanager')


def stage(destination):
    target=destination/'org/kde/plasma/private/containmentlayoutmanager'
    target.mkdir(parents=True,exist_ok=True)
    for file in SOURCE.iterdir():
        out=target/file.name
        if out.is_symlink():out.unlink()
        if file.suffix=='.so':out.symlink_to(file)
        elif file.is_file():shutil.copy2(file,out)
    qml=(target/'BasicAppletContainer.qml').read_text()
    qml=qml.replace('property bool blurEnabled: false','property bool blurEnabled: false\n        property bool sequoiaGlass: false',1)
    qml=qml.replace('function bindBlurEnabled() {','function bindBlurEnabled() {\n            sequoiaGlass = hasElementPrefix("sequoia-liquid-glass");',1)
    needle='''                blurEnabled: true
                blur: 1
                blurMax: 128'''
    replacement='''                // The existing blur is preserved. Refract its wallpaper texture
                // before the outer rounded mask and before applet contents.
                layer.enabled: background.sequoiaGlass && appletContainer.rotation === 0 && appletContainer.scale === 1
                layer.effect: ContainmentLayoutManager.GlassBackdrop {}
                blurEnabled: true
                blur: 1
                blurMax: 128'''
    if needle not in qml:raise RuntimeError('Plasma widget background changed; review the overlay patch.')
    (target/'BasicAppletContainer.qml').write_text(qml.replace(needle,replacement,1))
    qmldir=(target/'qmldir').read_text()
    qmldir='\n'.join(line for line in qmldir.splitlines() if not line.startswith('prefer '))+'\nGlassBackdrop 1.0 GlassBackdrop.qml\n'
    (target/'qmldir').write_text(qmldir)
    for name in ('GlassBackdrop.qml','glass-widget.frag.qsb'):shutil.copy2(ROOT/'desktop/glass-widget'/name,target/name)
    return target


def entry(path,after):
    return {'path':str(path),'before':base64.b64encode(path.read_bytes()).decode() if path.exists() else None,
            'after':base64.b64encode(after).decode()}


def apply(restart=True):
    if STATE.exists():return
    if DROPIN.exists():raise RuntimeError('Existing widget-glass service override must be reviewed first.')
    stage(OVERLAY)
    theme=subprocess.check_output(['kreadconfig6','--file','plasmarc','--group','Theme','--key','name'],text=True).strip()
    if not theme.startswith('sequoia-eleven-'):raise RuntimeError('Select the current Sequoia theme before adding its widget shader.')
    edits=[]
    for variant in ('sequoia-eleven-light','sequoia-eleven-dark','sequoia-eleven-glass-light','sequoia-eleven-glass-dark'):
        for relative in ('widgets/background.svg','translucent/widgets/background.svg'):
            path=DATA/'plasma/desktoptheme'/variant/relative
            if not path.exists():continue
            before=path.read_text()
            marker='<rect id="sequoia-liquid-glass-center" x="850" y="1" width="1" height="1" fill="none"/>'
            if 'sequoia-liquid-glass-center' not in before:edits.append(entry(path,before.replace('</svg>',marker+'\n</svg>').encode()))
    pid=subprocess.check_output(['systemctl','--user','show','plasma-plasmashell.service','--property=MainPID','--value'],text=True).strip()
    environ=Path('/proc')/pid/'environ'
    values=dict(line.split(b'=',1) for line in environ.read_bytes().split(b'\0') if b'=' in line)
    previous=values.get(b'QML_IMPORT_PATH',b'').decode()
    paths=str(OVERLAY)+(os.pathsep+previous if previous else '')
    paths=paths.replace('\\','\\\\').replace('"','\\"').replace('%','%%')
    edits.append(entry(DROPIN,('[Service]\nEnvironment="QML_IMPORT_PATH='+paths+'"\n').encode()))
    STATE.parent.mkdir(parents=True,exist_ok=True);STATE.write_text(json.dumps({'files':edits},indent=2)+'\n')
    try:
        for e in edits:
            path=Path(e['path']);path.parent.mkdir(parents=True,exist_ok=True)
            temp=path.with_suffix('.glass-tmp');temp.write_bytes(base64.b64decode(e['after']));temp.replace(path)
        subprocess.run(['systemctl','--user','daemon-reload'],check=True)
        if restart:subprocess.run(['systemctl','--user','restart','plasma-plasmashell.service'],check=True)
    except Exception:
        restore(False);raise


def restore(logout=False):
    if not STATE.exists():return
    state=json.loads(STATE.read_text())
    for e in reversed(state['files']):
        path=Path(e['path'])
        if path.exists() and path.read_bytes()==base64.b64decode(e['after']):
            if e['before'] is None:path.unlink()
            else:
                temp=path.with_suffix('.glass-tmp');temp.write_bytes(base64.b64decode(e['before']));temp.replace(path)
    STATE.unlink()
    subprocess.run(['systemctl','--user','daemon-reload'],check=not logout)
    if not logout:subprocess.run(['systemctl','--user','restart','plasma-plasmashell.service'],check=True)

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('action',choices=['start','restore']);p.add_argument('--logout',action='store_true');a=p.parse_args()
    if a.action=='start':apply()
    else:restore(a.logout)
