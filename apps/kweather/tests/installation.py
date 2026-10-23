#!/usr/bin/env python3
"""Check launcher integrity, argument forwarding and reversible private installation."""
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import os
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='sequoia-weather-install-test-') as temp:
    base=Path(temp);data=base/'user data';bins=base/'user bin';bins.mkdir();(data/'applications').mkdir(parents=True)
    launcher=bins/'kweather';launcher.write_text('#!/bin/sh\nexit 19\n');launcher.chmod(0o755)
    desktop=data/'applications/org.kde.kweather.desktop';desktop.write_text('[Desktop Entry]\nName=Previous weather entry\n')
    command=['python3',str(ROOT/'tools/install-kweather.py'),'--skip-build','--prefix',str(data),'--bin-prefix',str(bins)]
    subprocess.run(command,check=True)
    spec=importlib.util.spec_from_loader('weather_launcher',SourceFileLoader('weather_launcher',str(launcher)))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    target=data/'sequoia-eleven/kweather';args=['--version','argument with spaces']
    assert module.command(data,args)==[str(target/'bin/kweather'),*args]
    env=dict(os.environ,QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='generic',DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-weather-version')
    env.pop('LD_PRELOAD',None);env.pop('LD_LIBRARY_PATH',None)
    assert '26.08.1' in subprocess.check_output([str(launcher),'--version'],env=env,text=True)
    dynamic=subprocess.check_output(['readelf','-d',str(target/'bin/kweather')],text=True)
    assert '$ORIGIN/../lib' in dynamic and '.staging' not in dynamic
    manifest=target/'build.json';original=manifest.read_bytes();state=json.loads(original)
    state['upstream_sha256']='updated-flatpak';manifest.write_text(json.dumps(state))
    assert module.command(data,args)==['flatpak','run','org.kde.kweather',*args]
    manifest.write_bytes(original)
    binary=target/'bin/kweather';original=binary.read_bytes();binary.write_bytes(original+b'changed')
    assert module.command(data,args)==['flatpak','run','org.kde.kweather',*args]
    result=subprocess.run([*command,'--remove','--check'],capture_output=True)
    assert result.returncode!=0, 'modified managed file must be preserved'
    binary.write_bytes(original)
    subprocess.run([*command,'--remove','--check'],check=True)
    subprocess.run([*command,'--remove'],check=True)
    assert launcher.read_text()=='#!/bin/sh\nexit 19\n'
    assert desktop.read_text()=='[Desktop Entry]\nName=Previous weather entry\n'
print('PASS: private runtime, relocatable library lookup, arguments with spaces, update/tamper fallback, edit protection and restored launchers')
