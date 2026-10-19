#!/usr/bin/env python3
"""Launch Plasma with the dashboard only while its source ABI is unchanged."""
import hashlib
import json
import os
from pathlib import Path
import sys

directory = Path(__file__).resolve().parent
environment = dict(os.environ)
try:
    manifest = json.loads((directory / 'manifest.json').read_text())
    compatible = bool(manifest['fingerprints']) and all(
        hashlib.sha256(Path(source).read_bytes()).hexdigest() == digest
        for source, digest in manifest['fingerprints'].items())
except (OSError, ValueError, KeyError):
    compatible = False
if compatible and (directory / 'tray-qml.so').is_file():
    previous = environment.get('LD_PRELOAD', '')
    environment['SEQUOIA_CONTROL_CENTER_PRELOAD'] = previous
    environment['LD_PRELOAD'] = str(directory / 'tray-qml.so') + (':' + previous if previous else '')
    environment['SEQUOIA_CONTROL_CENTER_DIR'] = str(directory)
else:
    print('Sequoia Control Center: Plasma changed; using the original tray. Rerun install-control-center.py.', file=sys.stderr)
command = sys.argv[1:] or ['/usr/bin/plasmashell', '--no-respawn']
os.execvpe(command[0], command, environment)
