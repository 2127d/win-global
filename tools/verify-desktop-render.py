#!/usr/bin/env python3
"""Check real rendering and isolation, using a snapshot of the current desktop."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
HELPER=ROOT/'apps/settings/appearance-desktop-render.py'
spec=importlib.util.spec_from_file_location('renderer',HELPER)
renderer=importlib.util.module_from_spec(spec);spec.loader.exec_module(renderer)
source=Path.home()/'.config'
def fingerprint(directory):
    return {name:hashlib.sha256((directory/name).read_bytes()).hexdigest() if (directory/name).exists() else None for name in renderer.CONFIG_FILES}
before=fingerprint(source)
output=ROOT/'.staging/desktop-render-verification';output.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='desktop-render-test-') as temp:
    snapshot=Path(temp)/'config';snapshot.mkdir()
    for name in renderer.CONFIG_FILES:
        if (source/name).is_file():
            (snapshot/name).parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source/name,snapshot/name)
    copied=fingerprint(snapshot)
    env=dict(os.environ,XDG_CONFIG_HOME=str(snapshot))
    for material,mode in [(material,mode) for material in ('solid','transparent','glass') for mode in ('light','dark')]:
        image=output/f'{material}-{mode}.png'
        subprocess.run(['python3',str(HELPER),'--material',material,'--mode',mode,'--width','3440','--height','1440','--output',str(image)],env=env,check=True,timeout=90)
        data=json.loads(Path(str(image)+'.json').read_text())
        assert set(data['applications'])=={'gwenview','weather'},data
        for name,app_id,x in [('gwenview','org.kde.gwenview',241),('weather','org.kde.kweather',1737)]:
            application=data['applications'][name]
            assert application['application']==app_id,data
            assert application['geometry']==dict(x=x,y=202,width=1445,height=1089),data
        assert data['desktop']['desktops']>=1 and data['desktop']['panels']>=1,data
        assert ('sequoia_liquid_glass_v6' in data['effects'])==(material=='glass'),data
        assert ('blur' in data['effects'])==(material=='transparent'),data
        assert (sum(data['palette'][:3])<384)==(mode=='dark'),data
        assert data['palette'][3]==(255 if material=='solid' else (192 if mode=='dark' else 184) if material=='transparent' else (168 if mode=='dark' else 148)),data
        assert image.stat().st_size>10000
        assert fingerprint(snapshot)==copied,'Source snapshot was modified'
    assert fingerprint(source)==before,'Live desktop configuration changed'
print('PASS: native Plasma, native Gwenview and Weather, reference placement, all six material/mode combinations and unchanged source/live configuration')
