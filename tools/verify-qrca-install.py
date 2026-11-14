#!/usr/bin/env python3
"""Verify reversible QRCA installation, both launchers and damaged-payload fallback."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('qrca_launch',root/'apps/qrca/launch.py')
launcher_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(launcher_module)
with tempfile.TemporaryDirectory(prefix='sequoia qrca install ') as temporary:
    base=Path(temporary);data=base/'data with spaces';bins=base/'bin with spaces'
    bins.mkdir();(data/'applications').mkdir(parents=True)
    originals={bins/'qrca':b'#!/bin/sh\necho previous launcher\n'}
    for name in ('org.kde.qrca.desktop','org.kde.qrca.wifi.desktop'):
        originals[data/'applications'/name]=b'[Desktop Entry]\nType=Application\nName=Previous QRCA\nExec=/usr/bin/qrca\n'
    for path,value in originals.items():path.write_bytes(value)
    command=[sys.executable,str(root/'tools/install-qrca.py'),'--prefix',str(data),'--bin-prefix',str(bins)]
    for _ in range(2):subprocess.run([*command,'--skip-build'],check=True,capture_output=True,text=True)
    target=data/'sequoia-eleven/qrca'
    args=['--wifi','--encode','text with spaces']
    assert launcher_module.command(data,Path('/usr/bin/qrca'),args)==[str(target/'qrca'),*args]
    for name in ('org.kde.qrca.desktop','org.kde.qrca.wifi.desktop'):
        path=data/'applications'/name
        subprocess.run(['desktop-file-validate',str(path)],check=True)
        assert str(bins/'qrca') in path.read_text()
        if '.wifi.' in name:assert '--wifi' in path.read_text()
    asset=target/'qml/org/sequoia/components/ActionPill.qml'
    original=asset.read_bytes();asset.write_bytes(original+b'\n// edited\n')
    assert launcher_module.command(data,Path('/usr/bin/qrca'),args)==['/usr/bin/qrca',*args]
    result=subprocess.run([*command,'--remove','--check'],capture_output=True)
    assert result.returncode!=0,'edited runtime must be protected'
    asset.write_bytes(original)
    subprocess.run([*command,'--remove','--check'],check=True,capture_output=True)
    subprocess.run([*command,'--remove'],check=True,capture_output=True)
    for path,value in originals.items():assert path.read_bytes()==value,path
    assert not (target/'qrca').exists()
print('PASS: install, refresh, both launchers, argument forwarding, damaged-payload fallback, edit protection and restoration')
