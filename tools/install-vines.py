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
ID='sequoia-vines'

def run(*args):
    return subprocess.check_output(args,text=True,stderr=subprocess.STDOUT,timeout=60).strip()

def effect(method):
    return run('qdbus-qt6','org.kde.KWin','/Effects','org.kde.kwin.Effects.'+method,ID)

def enabled(value):
    args=['kwriteconfig6','--file','kwinrc','--group','Plugins','--key',ID+'Enabled']
    run(*args,*(['--delete'] if value=='' else [value]))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=('install','remove','status'),nargs='?',default='install')
    args=parser.parse_args()
    if args.action=='status':
        print('Jungle Vines loaded:',effect('isEffectLoaded'))
        return
    data=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share').absolute()
    receipt=Receipt(data,'sequoia-eleven/vines-installation',lambda name:name.startswith('kwin/effects/'+ID+'/'))
    if args.action=='remove':
        if not receipt.manifest.exists():
            print('No tracked Jungle Vines installation.');return
        receipt.validate()
        activation=receipt.data.get('activation',{'enabled':'','loaded':False})
        effect('unloadEffect');receipt.uninstall();enabled(activation['enabled'])
        if activation['loaded']:effect('loadEffect')
        print('Removed Jungle Vines and restored the previous activation setting.')
        return
    subprocess.run(['cmake','-S',str(ROOT/'desktop/effects/vines'),'-B',str(ROOT/'build/vines'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True)
    subprocess.run(['cmake','--build',str(ROOT/'build/vines'),'--parallel','4'],check=True)
    receipt.data.setdefault('activation',{
        'enabled':run('kreadconfig6','--file','kwinrc','--group','Plugins','--key',ID+'Enabled'),
        'loaded':effect('isEffectLoaded')=='true'})
    with tempfile.TemporaryDirectory(prefix='sequoia-vines-install-') as temporary:
        payload=Path(temporary)
        shutil.copytree(ROOT/'build/vines/package',payload/'kwin/effects'/ID)
        receipt.install(payload)
    if effect('isEffectLoaded')!='true' and effect('loadEffect')!='true':
        raise RuntimeError('Jungle Vines was installed but KWin could not load it. Inspect the KWin journal; startup settings were not changed.')
    enabled('true')
    print('Jungle Vines is active. Meta+Ctrl+M toggles the canopy. Enabled for future logins.')

if __name__=='__main__':main()
