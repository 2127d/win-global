"""Persist the installed glass effect while retaining exact restore settings."""
import json
import os
from pathlib import Path

STATE=Path(os.environ.get('XDG_STATE_HOME',Path.home()/'.local/state'))/'sequoia-eleven/liquid-glass-persistent.json'
EFFECT='sequoia_liquid_glass_v6'
LEGACY=('sequoia_liquid_glass','sequoia_liquid_glass_v2','sequoia_liquid_glass_v3','sequoia_liquid_glass_v4','sequoia_liquid_glass_v5')


def enable():
    from app_glass import CONFIG,HOOK,read,write
    desired={EFFECT+'Enabled':'true','blurEnabled':'false',**{name+'Enabled':'false' for name in LEGACY}}
    if STATE.exists():
        state=json.loads(STATE.read_text())
        if state['config']!=str(CONFIG):raise RuntimeError('Persistent glass receipt belongs to another config directory.')
    else:
        state={'version':1,'config':str(CONFIG),'changes':[{'file':'kwinrc','group':'Plugins','key':key,
               'before':read(CONFIG/'kwinrc','Plugins',key),'after':value} for key,value in desired.items()]}
    # Upgrades retain the original baseline for old keys, and record newly
    # introduced plugin keys before changing them.
    recorded={entry['key']:entry for entry in state['changes']}
    for key,value in desired.items():
        if key in recorded:recorded[key]['after']=value
        else:state['changes'].append({'file':'kwinrc','group':'Plugins','key':key,
                  'before':read(CONFIG/'kwinrc','Plugins',key),'after':value})
    STATE.parent.mkdir(parents=True,exist_ok=True)
    temp=STATE.with_suffix('.tmp');temp.write_text(json.dumps(state,indent=2)+'\n');temp.replace(STATE)
    for key,value in desired.items():write(CONFIG/'kwinrc','Plugins',key,value)
    # Persistence explicitly replaces the previous session-only cleanup policy.
    HOOK.unlink(missing_ok=True)


def disable():
    if not STATE.exists():return
    from app_glass import CONFIG,restore_settings
    state=json.loads(STATE.read_text())
    if state['config']!=str(CONFIG):raise RuntimeError('Persistent glass receipt belongs to another config directory.')
    restore_settings(CONFIG,state['changes'])
    STATE.unlink()
