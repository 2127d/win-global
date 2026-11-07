#!/usr/bin/env python3
"""Switch the live KWin session between standard blur and the glass experiment.

The native plugin must already be installed. Start is session-only unless the
separate app-glass persistence control has enabled saved login activation.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

EFFECT = 'sequoia_liquid_glass_v6'
LEGACY = ('sequoia_liquid_glass', 'sequoia_liquid_glass_v2', 'sequoia_liquid_glass_v3', 'sequoia_liquid_glass_v4', 'sequoia_liquid_glass_v5')
QDBUS = '/usr/lib64/qt6/bin/qdbus'
STATE = Path(os.environ.get('XDG_STATE_HOME', Path.home()/'.local/state'))/'sequoia-eleven/liquid-glass-trial.json'

def call(method, *args):
    return subprocess.check_output([QDBUS, 'org.kde.KWin', '/Effects', 'org.kde.kwin.Effects.'+method, *args],text=True,timeout=10).strip()

def owner():
    return subprocess.check_output([QDBUS, 'org.freedesktop.DBus', '/org/freedesktop/DBus',
        'org.freedesktop.DBus.GetNameOwner', 'org.kde.KWin'],text=True,timeout=10).strip()

def loaded(name):
    return call('isEffectLoaded', name) == 'true'

def load(name):
    if not loaded(name) and call('loadEffect', name) != 'true':
        raise RuntimeError('KWin could not load '+name)

def restore():
    # Older restore launchers also undo the newer app palette trial.
    from app_glass import STATE as app_state
    if app_state.exists():
        subprocess.run([__import__('sys').executable,str(Path(__file__).with_name('app-glass-trial.py')),'restore'],check=True)
        return
    from glass_persistence import disable as disable_persistence
    disable_persistence()
    state = json.loads(STATE.read_text()) if STATE.exists() else None
    if not loaded(EFFECT) and not any(loaded(name) for name in LEGACY) and (state is None or state.get('owner') != owner()):
        STATE.unlink(missing_ok=True)
        print('Liquid glass is not active; nothing to restore.')
        return
    call('unloadEffect', EFFECT)
    for name in LEGACY:
        call('unloadEffect', name)
    if state is None or state['standard_blur_loaded']:
        load('blur')
    if loaded(EFFECT) or any(loaded(name) for name in LEGACY):
        raise RuntimeError('Liquid glass did not unload')
    STATE.unlink(missing_ok=True)
    print('Standard blur restored. Saved glass activation has been removed.')

def start():
    if loaded(EFFECT):
        call('unloadEffect','blur')
        for name in LEGACY:call('unloadEffect',name)
        call('reconfigureEffect',EFFECT)
        print('Liquid glass is already running in the live session.')
        return
    if EFFECT not in call('listOfEffects').splitlines():
        raise RuntimeError('Install the matching native liquid-glass plugin before starting the trial.')
    # Preserve the original baseline across an interrupted activation attempt.
    state = json.loads(STATE.read_text()) if STATE.exists() else None
    if state is None or state.get('owner') != owner():
        state = {'version': 1, 'standard_blur_loaded': loaded('blur'), 'started': time.time(), 'owner': owner()}
    STATE.parent.mkdir(parents=True,exist_ok=True)
    tmp=STATE.with_suffix('.tmp');tmp.write_text(json.dumps(state,indent=2)+'\n');tmp.replace(STATE)
    try:
        call('unloadEffect','blur')
        for name in LEGACY:
            call('unloadEffect', name)
        load(EFFECT)
        if loaded('blur') or not loaded(EFFECT):
            raise RuntimeError('The requested effect swap did not complete')
        time.sleep(1)
        if EFFECT not in call('activeEffects').splitlines():
            raise RuntimeError('The liquid-glass effect loaded but is not rendering')
    except Exception:
        restore()
        raise
    print('Liquid glass is active in the live desktop. Standard blur is unloaded.')
    from glass_persistence import STATE as persistent_state
    print('Saved login activation is enabled.' if persistent_state.exists() else 'Trial is session-only; persistent KWin settings were not changed.')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('action',choices=['start','restore','status'])
    p.add_argument('--notify',action='store_true')
    a=p.parse_args()
    if a.action=='start':start()
    elif a.action=='restore':restore()
    else:print(json.dumps({'persistent':__import__('glass_persistence').STATE.exists(),'glass_loaded':loaded(EFFECT),'legacy_loaded':any(loaded(name) for name in LEGACY),'standard_blur_loaded':loaded('blur'),'glass_active':EFFECT in call('activeEffects').splitlines()},indent=2))
    if a.notify and a.action!='status':
        from glass_persistence import STATE as persistent_state
        message=('Glass will load at future logins.' if persistent_state.exists() else 'Glass is enabled for this session.') if a.action=='start' else 'Saved glass activation is disabled.'
        subprocess.run(['notify-send','--app-name=Sequoia Glass','Liquid glass enabled' if a.action=='start' else 'Standard blur restored',message],check=False)

if __name__=='__main__':main()
