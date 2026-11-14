"""Build separate Plasma glass assets and reversible Quick toolbar file edits.

The existing native compositor supplies refraction for popup windows. Desktop
widget SVGs supply the polished rim over Plasma's existing wallpaper blur.
"""
import base64
import copy
import json
from pathlib import Path
import shutil
import subprocess
import xml.etree.ElementTree as ET
from build import Svg

ROOT=Path(__file__).resolve().parents[1]
DATA=Path.home()/'.local/share'


def glass_frame(svg, prefix, dark, radius=24, margin=12, widget=False):
    tint='#141b29' if dark else '#f2f6ff'
    opacity=(0.30 if dark else 0.28) if widget else (0.44 if dark else 0.42)
    start=len(svg.parts)
    svg.frame(prefix,tint,radius,'#ffffff',opacity,margin,stroke=1,border_opacity=0.48)
    # Merge inset rings into the same nine slices. Their total bounds stay equal,
    # so Plasma stretches only the straight edges and never distorts the corners.
    groups={ET.fromstring(s).get('id'):i for i,s in enumerate(svg.parts[start:],start) if s.startswith('<g ')}
    for inset,width,color,alpha in [(1,2,'#ffffff',0.14),(3,1,'#13284b',0.12)]:
        layer=Svg();layer.frame(prefix,'#000000',radius-inset,color,0,stroke=width,inset=inset,border_opacity=alpha)
        for fragment in layer.parts:
            if not fragment.startswith('<g '):continue
            element=ET.fromstring(fragment);index=groups[element.get('id')]
            target=ET.fromstring(svg.parts[index])
            for child in element:target.append(copy.deepcopy(child))
            svg.parts[index]=ET.tostring(target,encoding='unicode')


def build_theme(base, destination, dark):
    if destination.exists():shutil.rmtree(destination)
    shutil.copytree(base,destination)
    metadata=json.loads((destination/'metadata.json').read_text())
    metadata['KPlugin'].update(Id=destination.name,Name='Sequoia Glass Trial '+('Dark' if dark else 'Light'))
    (destination/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    # Opaque/no-compositor fallbacks and desktop panels retain their original assets.
    for folder in ('','translucent'):
        for filename in ('dialogs/background','widgets/background','widgets/tooltip'):
            s=Svg();radius=16 if filename.endswith('tooltip') else 24
            margin=8 if radius==16 else 12
            glass_frame(s,'',dark,radius,margin)
            s.frame('mask','#ffffff',radius)
            if filename=='widgets/background':
                glass_frame(s,'blurred',dark,radius,margin,True)
                s.frame('blurred-mask','#ffffff',radius)
            s.shadow(radius);s.save(destination/folder/(filename+'.svg'))
    return destination.name


def prepare(dark):
    name='sequoia-eleven-glass-'+('dark' if dark else 'light')
    base=DATA/'plasma/desktoptheme'/('sequoia-eleven-'+('dark' if dark else 'light'))
    if not base.exists():base=ROOT/'themes/plasma/desktoptheme'/base.name
    build_theme(base,DATA/'plasma/desktoptheme'/name,dark)
    install_rim_module()
    return name, quick_changes()


def install_rim_module():
    build=ROOT/'.staging/glass-material'
    subprocess.run(['cmake','-S',str(ROOT/'desktop/glass-material'),'-B',str(build),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True)
    subprocess.run(['cmake','--build',str(build),'--parallel','4'],check=True)
    target=DATA/'sequoia-eleven/quick-islands/qml/org/sequoia/glass'
    target.mkdir(parents=True,exist_ok=True)
    temp=target/'libsequoiaglassplugin.so.tmp'
    shutil.copy2(build/'libsequoiaglassplugin.so',temp);temp.replace(target/'libsequoiaglassplugin.so')
    (target/'qmldir').write_text('module org.sequoia.glass\nplugin sequoiaglassplugin\n')


def quick_changes(originals=None):
    from quick_islands import background
    changes=[]
    folder=DATA/'sequoia-eleven/quick-islands/qml/org/kde/desktop'
    for name in ('ToolBar.qml','Popup.qml'):
        path=folder/name
        if not path.exists():continue
        before=(originals or {}).get(str(path),path.read_text())
        text=before.replace('import QtQuick\n','import QtQuick\nimport org.sequoia.glass as Glass\n',1)
        if name=='ToolBar.qml':
            # Keep the shared opaque island, including when upgrading an active
            # trial whose previous override removed its fill.
            after=before
        else:
            needle='        border.width: 1'
            if text.count(needle)!=1:raise RuntimeError('Quick popup changed; review its glass patch.')
            after=text.replace(needle,'        border.width: Kirigami.Theme.backgroundColor.a < 1 ? 0 : 1\n        Glass.Rim {\n            anchors.fill: parent\n            radius: parent.radius\n            visible: Kirigami.Theme.backgroundColor.a < 1\n        }',1)
        changes.append({'path':str(path),'before':base64.b64encode(before.encode()).decode(),'after':base64.b64encode(after.encode()).decode()})
    return changes


def refresh_quick():
    from app_glass import STATE
    if not STATE.exists():return
    install_rim_module()
    state=json.loads(STATE.read_text())
    old={e['path']:e for e in state.get('files',[])}
    for path,e in old.items():
        if Path(path).read_bytes()!=base64.b64decode(e['after']):
            raise RuntimeError('Glass overlay was modified externally: '+path)
    changes=quick_changes({path:base64.b64decode(e['before']).decode() for path,e in old.items()})
    # Keep the original restoration baseline while upgrading the active patch.
    state['files']=[e for e in state.get('files',[]) if e['path'] not in {c['path'] for c in changes}]+changes
    temp=STATE.with_suffix('.tmp');temp.write_text(json.dumps(state,indent=2)+'\n');temp.replace(STATE)
    apply_files(changes)


def apply_files(changes,restore=False):
    for edit in changes:
        path=Path(edit['path']);before=base64.b64decode(edit['before']);after=base64.b64decode(edit['after'])
        if not restore or (path.exists() and path.read_bytes()==after):
            tmp=path.with_suffix('.glass-tmp');tmp.write_bytes(before if restore else after);tmp.replace(path)
