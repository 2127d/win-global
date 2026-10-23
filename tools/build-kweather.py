#!/usr/bin/env python3
"""Build Sequoia Weather and its private KDE weather engine."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from installer_platform import build_jobs

ROOT=Path(__file__).resolve().parents[1]
VERSION='26.08.1'

def stock():
    location=Path(subprocess.check_output(['flatpak','info','--show-location','org.kde.kweather'],text=True).strip())
    manifest=json.loads((location/'files/manifest.json').read_text())
    sources={m['name']:m['sources'][0] for m in manifest['modules']}
    if not sources['kweather']['url'].endswith(f'/kweather-{VERSION}.tar.xz'):
        raise RuntimeError(f'This makeover requires KWeather {VERSION} from Flatpak.')
    return location/'files/bin/kweather'

def build(folder,sdk=None):
    stock()
    folder=folder.resolve();prefix=folder/'install';prefix.mkdir(parents=True,exist_ok=True)
    jobs=str(build_jobs())
    print(f"Building Weather with {jobs} parallel jobs.",flush=True)
    for name,extra in [('kweathercore',[]),('kweather',['-DBUILD_PLASMOID=OFF','-DBUILD_KRUNNER=OFF'])]:
        source=ROOT/f'apps/kweather/{name}-{VERSION}'
        target=folder/('core' if name=='kweathercore' else 'app')
        command=['cmake','--fresh','-S',str(source),'-B',str(target),'-G','Ninja',
            '-DBUILD_TESTING=OFF','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_INSTALL_PREFIX='+str(prefix),
            '-DKDE_INSTALL_LIBDIR=lib','-DCMAKE_INSTALL_RPATH=$ORIGIN/../lib',
            '-DCMAKE_PREFIX_PATH='+str(prefix)+((';'+str(sdk.resolve())) if sdk else ''),*extra]
        if sdk: command.append('-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH='+str(sdk.resolve()))
        subprocess.run(command,check=True)
        subprocess.run(['cmake','--build',str(target),'--parallel',jobs],check=True)
        subprocess.run(['cmake','--install',str(target)],check=True)
    return prefix

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'.staging/kweather-release')
    parser.add_argument('--sdk',type=Path)
    args=parser.parse_args();print(build(args.build_dir,args.sdk))
