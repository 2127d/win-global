#!/usr/bin/env python3
"""Launch the private Sequoia Weather build; fall back to Flatpak after updates."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
INSTALL_DATA=Path('__SEQUOIA_KWEATHER_DATA__')

def command(data,arguments):
    target=data/'sequoia-eleven/kweather'
    try:
        build=json.loads((target/'build.json').read_text())
        deployed=Path(subprocess.check_output(['flatpak','info','--show-location','org.kde.kweather'],text=True,stderr=subprocess.DEVNULL).strip())
        valid=hashlib.sha256((deployed/'files/bin/kweather').read_bytes()).hexdigest()==build['upstream_sha256']
        for name,digest in build['payload'].items():
            valid=valid and hashlib.sha256((target/name).read_bytes()).hexdigest()==digest
        valid=valid and os.access(target/'bin/kweather',os.X_OK)
    except (OSError,ValueError,KeyError,subprocess.SubprocessError): valid=False
    if not valid:
        print('The Sequoia Weather makeover needs rebuilding; opening the installed Flatpak.',file=sys.stderr)
        return ['flatpak','run','org.kde.kweather',*arguments]
    return [str(target/'bin/kweather'),*arguments]

if __name__=='__main__':
    argv=command(INSTALL_DATA,sys.argv[1:])
    if argv[0]!='flatpak':
        target=INSTALL_DATA/'sequoia-eleven/kweather'
        os.environ['XDG_DATA_DIRS']=str(target/'share')+':'+os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share')
        os.environ['QML_IMPORT_PATH']=str(target/'lib/qml')+':'+os.environ.get('QML_IMPORT_PATH','')
    os.execvp(argv[0],argv)
