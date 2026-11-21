#!/usr/bin/env python3
"""Install, inspect, or remove the per-user Jungle Vines KWin effect."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from theme_files import Receipt

ROOT=Path(__file__).resolve().parents[1]
ID='sequoia-vines-v7'
LEGACY_ID='sequoia-vines-v6'

def run(*args):
    return subprocess.check_output(args,text=True,stderr=subprocess.STDOUT,timeout=60).strip()

def effect(method, name=ID):
    return run('qdbus-qt6','org.kde.KWin','/Effects','org.kde.kwin.Effects.'+method,name)

def enabled(value, name=ID):
    args=['kwriteconfig6','--file','kwinrc','--group','Plugins','--key',name+'Enabled']
    run(*args,*(['--delete'] if value=='' else [value]))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('install','remove','status'),nargs='?',default='install')
    args=parser.parse_args()
    if args.action=='status':
        print('Jungle Vines loaded:',effect('isEffectLoaded'))
        return
    data=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share').absolute()
    receipt=Receipt(data,'sequoia-eleven/vines-v7-installation',lambda name:name.startswith('kwin/effects/'+ID+'/'))
    if args.action=='remove':
        if not receipt.manifest.exists():
            print('No tracked Jungle Vines installation.');return
        receipt.validate()
        activation=receipt.data.get('activation',{'enabled':'','loaded':False})
        effect('unloadEffect');receipt.uninstall();enabled(activation['enabled'])
        if activation['loaded']:effect('loadEffect')
        print('Removed Jungle Vines and restored the previous activation setting.')
        return
    subprocess.run(['cmake','-S',str(ROOT/'desktop/effects/vines'),'-B',str(ROOT/'build/vines-v7'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True)
    subprocess.run(['cmake','--build',str(ROOT/'build/vines-v7'),'--parallel','4'],check=True)
    legacy=Receipt(data,'sequoia-eleven/vines-v6-installation',lambda name:name.startswith('kwin/effects/'+LEGACY_ID+'/'))
    if legacy.manifest.exists():legacy.validate()
    receipt.data.setdefault('activation',{
        'enabled':run('kreadconfig6','--file','kwinrc','--group','Plugins','--key',ID+'Enabled'),
        'loaded':effect('isEffectLoaded')=='true'})
    with tempfile.TemporaryDirectory(prefix='sequoia-vines-install-') as temporary:
        payload=Path(temporary)
        shutil.copytree(ROOT/'build/vines-v7/package',payload/'kwin/effects'/ID)
        receipt.install(payload)
    legacy_loaded=legacy.manifest.exists() and effect('isEffectLoaded',LEGACY_ID)=='true'
    if legacy_loaded:effect('unloadEffect',LEGACY_ID)
    if effect('isEffectLoaded')!='true' and effect('loadEffect')!='true':
        if legacy_loaded:effect('loadEffect',LEGACY_ID)
        raise RuntimeError('Jungle Vines was installed but KWin could not load it. Inspect the KWin journal; startup settings were not changed.')
    enabled('true')
    if legacy.manifest.exists():
        previous=legacy.data.get('activation',{'enabled':'','loaded':False})
        backup=data/'sequoia-eleven/vines-v6-upgrade-backup'
        if not backup.exists():
            shutil.copytree(legacy.state,backup/'receipt')
            shutil.copytree(data/'kwin/effects'/LEGACY_ID,backup/'package')
        legacy.uninstall()
        enabled(previous['enabled'],LEGACY_ID)
    print('Jungle Vines is active. Meta+Ctrl+M toggles the canopy. Enabled for future logins.')

if __name__=='__main__':main()
