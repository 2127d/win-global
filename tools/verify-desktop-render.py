#!/usr/bin/env python3
"""Check real rendering and isolation, using a snapshot of the current desktop."""
import configparser
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
        if (source/name).is_file(): shutil.copy2(source/name,snapshot/name)
    copied=fingerprint(snapshot)
    env=dict(os.environ,XDG_CONFIG_HOME=str(snapshot))
    for material,mode in [('solid','light'),('transparent','dark'),('glass','light')]:
        image=output/f'{material}-{mode}.png'
        subprocess.run(['python3',str(HELPER),'--material',material,'--mode',mode,'--width','1600','--height','900','--output',str(image)],env=env,check=True,timeout=90)
        data=json.loads(Path(str(image)+'.json').read_text())
        assert data['application']['application']=='org.kde.dolphin',data
        assert data['desktop']['desktops']>=1 and data['desktop']['panels']>=1,data
        assert ('sequoia_liquid_glass_v6' in data['effects'])==(material=='glass'),data
        assert ('blur' in data['effects'])==(material=='transparent'),data
        assert (sum(data['palette'][:3])<384)==(mode=='dark'),data
        assert data['palette'][3]==(255 if material=='solid' else 192 if material=='transparent' else 148),data
        assert image.stat().st_size>10000
        assert fingerprint(snapshot)==copied,'Source snapshot was modified'
    assert fingerprint(source)==before,'Live desktop configuration changed'
print('PASS: native Plasma, native Dolphin, all three compositor materials, light/dark palettes and unchanged source/live configuration')
