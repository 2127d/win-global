#!/usr/bin/env python3
"""Install/remove only the separate Appearance Hub preview. Never replace System Settings."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, default=ROOT/'.staging/settings-hub-build/bin/systemsettings')
parser.add_argument('--prefix', type=Path, default=Path.home()/'.local/share')
parser.add_argument('--bin-prefix', type=Path, default=Path.home()/'.local/bin')
parser.add_argument('--remove', action='store_true')
args = parser.parse_args()
target = args.prefix/'sequoia-eleven/settings-hub-preview'
launcher = args.bin_prefix/'settings-appearance-preview'
desktop = args.prefix/'applications/org.sequoia.AppearanceHubPreview.desktop'
if args.remove:
    # Undo a kept test as well; per-key ownership protects later manual edits.
    helper = target/'appearance-material.py'
    if helper.exists(): subprocess.run(['python3',str(helper),'restore'], check=True)
    launcher.unlink(missing_ok=True)
    desktop.unlink(missing_ok=True)
    if target.exists(): shutil.rmtree(target)
    print('Removed Appearance Hub Preview. Existing System Settings was untouched.')
    raise SystemExit(0)
if not args.binary.is_file(): raise SystemExit('Build the preview first.')
module=args.binary.parent/'qml/org/sequoia/components'
if not (module/'qmldir').is_file() or not (module/'libsequoiacomponentsplugin.so').is_file():
    raise SystemExit('Build the shared component library with the preview first.')
target.mkdir(parents=True,exist_ok=True)
args.bin_prefix.mkdir(parents=True,exist_ok=True)
desktop.parent.mkdir(parents=True,exist_ok=True)
for source,name in [(args.binary,'systemsettings'),(ROOT/'apps/settings/appearance-material.py','appearance-material.py'),(ROOT/'apps/settings/appearance-desktop-render.py','appearance-desktop-render.py')]:
    temp=target/(name+'.new');shutil.copy2(source,temp);temp.replace(target/name)
# Ship the shared module next to the preview so it is independent of checkout paths.
for source in module.rglob('*'):
    if source.is_file():
        destination=target/'qml/org/sequoia/components'/source.relative_to(module)
        destination.parent.mkdir(parents=True,exist_ok=True)
        temp=destination.with_name(destination.name+'.new');shutil.copy2(source,temp);temp.replace(destination)
manifest = {'upstream_sha256':hashlib.sha256(Path('/usr/bin/systemsettings').read_bytes()).hexdigest(),'version':'6.8.2'}
(target/'build.json').write_text(json.dumps(manifest,indent=2)+'\n')
launcher.write_text('''#!/usr/bin/env python3
import hashlib,json,os,subprocess,sys
from pathlib import Path
TARGET=Path(%r)
STOCK=Path('/usr/bin/systemsettings')
manifest=json.loads((TARGET/'build.json').read_text())
if manifest['upstream_sha256'] != hashlib.sha256(STOCK.read_bytes()).hexdigest():
    print('KDE changed since this preview was built. Opening stock System Settings.',file=sys.stderr)
    os.execv(str(STOCK),[str(STOCK),*sys.argv[1:]])
subprocess.run([sys.executable,str(TARGET/'appearance-material.py'),'recover'],check=True,stdout=subprocess.DEVNULL)
os.environ['SEQUOIA_APPEARANCE_HUB']='1'
argv=[str(TARGET/'systemsettings'),*sys.argv[1:]]
overlay=TARGET.parent/'quick-islands/sequoia-quick'
if overlay.is_file() and not os.environ.get('QML_IMPORT_PATH'): argv.insert(0,str(overlay))
os.execv(argv[0],argv)
''' % str(target))
launcher.chmod(0o755)
# Desktop Exec quoting: double quotes also escape reserved literal characters.
quoted = '"'+str(launcher).replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')+'"'
desktop.write_text('[Desktop Entry]\nType=Application\nName=Appearance Hub Preview\nComment=Reversible Settings redesign with every KDE module\nIcon=preferences-desktop-theme\nExec='+quoted+'\nTerminal=false\nCategories=Settings;DesktopSettings;\nStartupNotify=true\n')
print(launcher)
