#!/usr/bin/env python3
"""Launch the intact private Weather runtime independently of Flatpak updates."""
import hashlib
import json
import os
from pathlib import Path
import sys
INSTALL_DATA=Path('__SEQUOIA_KWEATHER_DATA__')

def command(data,arguments):
    target=data/'sequoia-eleven/kweather'
    try:
        build=json.loads((target/'build.json').read_text())
        # The native app bundles its own engine; the Flatpak is only a fallback.
        valid='bin/kweather' in build['payload']
        for name,digest in build['payload'].items():
            valid=valid and hashlib.sha256((target/name).read_bytes()).hexdigest()==digest
        valid=valid and os.access(target/'bin/kweather',os.X_OK)
    except (OSError,ValueError,KeyError): valid=False
    if not valid:
        print('The Sequoia Weather installation is missing or modified; opening the installed Flatpak.',file=sys.stderr)
        return ['flatpak','run','org.kde.kweather',*arguments]
    return [str(target/'bin/kweather'),*arguments]

if __name__=='__main__':
    argv=command(INSTALL_DATA,sys.argv[1:])
    if argv[0]!='flatpak':
        target=INSTALL_DATA/'sequoia-eleven/kweather'
        os.environ['XDG_DATA_DIRS']=str(target/'share')+':'+os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share')
        os.environ['QML_IMPORT_PATH']=str(target/'lib/qml')+':'+os.environ.get('QML_IMPORT_PATH','')
    os.execvp(argv[0],argv)
