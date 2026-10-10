#!/usr/bin/env python3
"""Restore a settings snapshot made by apply-desktop.py; plugins remain installed."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('backup',type=Path)
args=parser.parse_args()
manifest=json.loads((args.backup/'manifest.json').read_text())
roots = [Path.home().resolve()] + [Path(os.environ.get(variable) or Path.home()/default).resolve()
         for variable, default in [('XDG_CONFIG_HOME', '.config'),
                                   ('XDG_DATA_HOME', '.local/share')]]
for entry in manifest:
    path=Path(entry['path'])
    source=args.backup/entry['copy']
    # XDG settings/data may live outside HOME. Never restore another account's
    # snapshot or a destination outside the current account's configured roots.
    resolved = path.resolve()
    if not path.is_absolute() or not any(resolved != root and resolved.is_relative_to(root) for root in roots):
        raise RuntimeError(f'Refusing a snapshot destination outside the current home/configuration/data directories: {path}')
    if not source.resolve().is_relative_to(args.backup.resolve()):
        raise RuntimeError(f'Invalid snapshot source: {source}')
    if entry['exists']:
        if not source.exists(): raise RuntimeError(f'Incomplete snapshot: {source}')
for entry in manifest:
    path=Path(entry['path']);source=args.backup/entry['copy']
    if path.is_dir(): shutil.rmtree(path)
    elif path.exists(): path.unlink()
    if entry['exists']:
        path.parent.mkdir(parents=True,exist_ok=True)
        if entry['directory']:shutil.copytree(source,path)
        else:shutil.copy2(source,path)
if any('/fonts/sequoia-eleven/' in entry['path'] for entry in manifest):
    subprocess.run(['fc-cache', '-f'], check=False)
subprocess.run(['qdbus-qt6','org.kde.KWin','/KWin','org.kde.KWin.reconfigure'],check=False)
for change in [0,2]:
    subprocess.run(['dbus-send','--session','--type=signal','/KGlobalSettings','org.kde.KGlobalSettings.notifyChange',f'int32:{change}','int32:0'],check=False)
print('Previous settings restored. Log out and back in to refresh all running components.')
