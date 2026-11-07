"""Reversible app-canvas and compositor settings for the liquid-glass trial."""
import configparser
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
CONFIG=Path(os.environ.get('XDG_CONFIG_HOME',Path.home()/'.config'))
STATE=Path(os.environ.get('XDG_STATE_HOME',Path.home()/'.local/state'))/'sequoia-eleven/app-glass-trial.json'
HOOK=CONFIG/'plasma-workspace/shutdown/sequoia-app-glass-trial.sh'
PRESET={'BlurStrength':'4','NoiseStrength':'0','Saturation':'112','Refraction':'34','RimWidth':'96','LensStrength':'0.09','EdgeClarity':'0.75','EdgeThickness':'7','LightStrength':'0.34','LightAzimuth':'225','LightElevation':'50'}

def read(path,group,key):
    p=configparser.ConfigParser(interpolation=None,strict=False,allow_no_value=True);p.optionxform=str;p.read(path)
    return p.get(group,key,fallback=None)

def write(path,group,key,value):
    subprocess.run(['kwriteconfig6','--file',str(path),'--group',group,'--key',key,*(['--delete'] if value is None else [value])],check=True)
    if value is None:
        # KConfig can emit a deletion marker for an inherited default. The
        # receipt's absent key means restore inheritance, not suppress it.
        lines=path.read_text().splitlines(keepends=True);section=None;kept=[]
        for line in lines:
            stripped=line.strip()
            if stripped.startswith('[') and stripped.endswith(']'):section=stripped[1:-1]
            if section==group and stripped in (key+'[$d]',key+'[$d]='):continue
            kept.append(line)
        if kept!=lines:
            tmp=path.with_suffix('.glass-tmp');tmp.write_text(''.join(kept));tmp.replace(path)


def edits(config, surfaces=False):
    result=[]
    for group in ('Colors:Window','Colors:Header'):
        for key in ('BackgroundNormal','BackgroundAlternate'):
            old=read(config/'kdeglobals',group,key)
            if old is None:continue
            parts=old.split(',')
            if len(parts)!=4 or not all(x.isdigit() for x in parts) or int(parts[3])>=255:continue
            # Only already-translucent canvas roles; no whole-window opacity.
            dark=sum(map(int,parts[:3]))/3<128
            value=','.join(parts[:3]+[str(168 if dark else 148)])
            result.append({'file':'kdeglobals','group':group,'key':key,'before':old,'after':value})
    if not result:raise RuntimeError('Select a transparent Sequoia palette before starting the app glass trial.')
    for key,value in PRESET.items():
        result.append({'file':'kwinrc','group':'Effect-sequoia_liquid_glass','key':key,'before':read(config/'kwinrc','Effect-sequoia_liquid_glass',key),'after':value})
    if surfaces:
        window_roles={e['key']:e['after'] for e in result if e['group']=='Colors:Window'}
        for e in result:
            if e['group']=='Colors:Header' and e['key'] in window_roles:e['after']=window_roles[e['key']]
        canvas=next(e['after'] for e in result if e['group']=='Colors:Window' and e['key']=='BackgroundNormal')
        for key in ('activeBackground','activeBlend','inactiveBackground','inactiveBlend'):
            result.append({'file':'kdeglobals','group':'WM','key':key,'before':read(config/'kdeglobals','WM',key),'after':canvas})
        for file,group,key,value in [('kwinrc','org.kde.kdecoration2','library','org.kde.sequoiaeleven.glass-adjustable'),
                                      ('kdeglobals','KDE','widgetStyle','SequoiaElevenGlass'),
                                      ('sequoiaelevenrc','Windeco','OtherCornerRadius','16'),
                                      ('sequoiaelevenrc','Style','MenuOpacity','44')]:
            result.append({'file':file,'group':group,'key':key,'before':read(config/file,group,key),'after':value})
    return result

def apply_settings(config,changes):
    for e in changes:write(config/e['file'],e['group'],e['key'],e['after'])

def restore_settings(config,changes):
    for e in changes:
        # A later manual palette or shader adjustment wins over our receipt.
        if read(config/e['file'],e['group'],e['key'])==e['after']:
            write(config/e['file'],e['group'],e['key'],e['before'])

def notify_palette():
    subprocess.run(['dbus-send','--session','--type=signal','/KGlobalSettings','org.kde.KGlobalSettings.notifyChange','int32:0','int32:0'],check=True)

def shader(action):
    subprocess.run([sys.executable,str(ROOT/'tools/liquid-glass-trial.py'),action],check=True)

def refresh_surfaces():
    subprocess.run(['/usr/lib64/qt6/bin/qdbus','org.kde.KWin','/KWin','org.kde.KWin.reconfigure'],check=True)
    subprocess.run(['dbus-send','--session','--type=signal','/KGlobalSettings','org.kde.KGlobalSettings.notifyChange','int32:2','int32:0'],check=True)

def match_konsole_canvas():
    """Match the user's default terminal tint, retaining its ANSI colors."""
    if not STATE.exists():return
    data=Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local/share'))/'konsole'
    profile=read(CONFIG/'konsolerc','Desktop Entry','DefaultProfile')
    if not profile or Path(profile).name!=profile:return
    scheme=read(data/profile,'Appearance','ColorScheme')
    if not scheme or Path(scheme).name!=scheme:return
    path=data/(scheme+'.colorscheme')
    # Only existing user schemes; never edit a distribution-owned scheme.
    if not path.is_file():return
    canvas=read(CONFIG/'kdeglobals','Colors:Window','BackgroundNormal')
    if not canvas:return
    channels=canvas.split(',')
    if len(channels)!=4 or not all(x.isdigit() for x in channels):return
    # Custom terminal color choices stay independent. Match opacity when the
    # terminal already shares the app canvas color.
    if read(path,'Background','Color')!=','.join(channels[:3]):return
    value=format(int(channels[3])/255,'.12g')
    state=json.loads(STATE.read_text())
    if state['config']!=str(CONFIG):raise RuntimeError('Trial receipt belongs to another config directory.')
    entry=next((e for e in state['changes'] if e['file']==str(path) and e['group']=='General' and e['key']=='Opacity'),None)
    if entry:entry['after']=value
    else:state['changes'].append({'file':str(path),'group':'General','key':'Opacity','before':read(path,'General','Opacity'),'after':value})
    temp=STATE.with_suffix('.tmp');temp.write_text(json.dumps(state,indent=2)+'\n');temp.replace(STATE)
    write(path,'General','Opacity',value)

def start():
    from glass_persistence import STATE as persistent_state, enable as persist
    keep_persistent=persistent_state.exists()
    if STATE.exists():
        state=json.loads(STATE.read_text())
        chrome_matches=(read(CONFIG/'kdeglobals','KDE','widgetStyle')=='SequoiaElevenGlass'
            and read(CONFIG/'kwinrc','org.kde.kdecoration2','library')=='org.kde.sequoiaeleven.glass-adjustable')
        if state.get('surfaces') and chrome_matches:
            shader('start')
            match_konsole_canvas()
            if read(CONFIG/'plasmarc','Theme','name') in ('sequoia-eleven-glass-light','sequoia-eleven-glass-dark'):
                from widget_glass import apply as apply_widgets
                apply_widgets()
            if keep_persistent:persist()
            print('App glass is already configured.');return
        # Upgrade an earlier trial or reapply after a user-selected global theme.
        # Restore only values that still equal ours, retaining the new palette.
        restore(False, refresh_widgets=False)
    if HOOK.exists():raise RuntimeError('An existing trial cleanup script must be restored first.')
    from glass_surfaces import prepare,apply_files
    changes=edits(CONFIG,surfaces=True)
    dark=sum(map(int,read(CONFIG/'kdeglobals','Colors:Window','BackgroundNormal').split(',')[:3]))/3<128
    theme,files=prepare(dark)
    previous_theme=subprocess.check_output(['kreadconfig6','--file','plasmarc','--group','Theme','--key','name','--default','default'],text=True).strip()
    STATE.parent.mkdir(parents=True,exist_ok=True)
    STATE.write_text(json.dumps({'version':2,'config':str(CONFIG),'changes':changes,'surfaces':True,'files':files,'theme':theme,'previous_theme':previous_theme,'theme_override':read(CONFIG/'plasmarc','Theme','name')},indent=2)+'\n')
    HOOK.parent.mkdir(parents=True,exist_ok=True)
    HOOK.write_text('#!/bin/sh\nexec '+shlex.quote(sys.executable)+' '+shlex.quote(str(ROOT/'tools/app-glass-trial.py'))+' restore --logout\n')
    HOOK.chmod(0o700)
    try:
        apply_settings(CONFIG,changes)
        apply_files(files)
        subprocess.run(['plasma-apply-desktoptheme',theme],check=True)
        refresh_surfaces()
        shader('start')
        subprocess.run(['/usr/lib64/qt6/bin/qdbus','org.kde.KWin','/Effects','org.kde.kwin.Effects.reconfigureEffect','sequoia_liquid_glass_v6'],check=True)
        notify_palette()
        match_konsole_canvas()
        from widget_glass import apply as apply_widgets
        apply_widgets()
        if keep_persistent:persist()
    except Exception:
        restore(False);raise
    print('App glass applied. Saved login activation is enabled.' if keep_persistent else 'App glass trial applied. Restore control and logout cleanup are ready.')

def restore(logout=False, refresh_widgets=True):
    from glass_persistence import STATE as persistent_state, disable as disable_persistence
    if logout and persistent_state.exists():
        return
    disable_persistence()
    from widget_glass import restore as restore_widgets
    restore_widgets(logout or not refresh_widgets)
    if STATE.exists():
        state=json.loads(STATE.read_text())
        if state['config']!=str(CONFIG):raise RuntimeError('Trial receipt belongs to another config directory.')
        restore_settings(CONFIG,state['changes'])
        if state.get('surfaces'):
            from glass_surfaces import apply_files
            apply_files(state.get('files',[]),restore=True)
            if read(CONFIG/'plasmarc','Theme','name')==state['theme']:
                if not logout:subprocess.run(['plasma-apply-desktoptheme',state['previous_theme']],check=True)
                write(CONFIG/'plasmarc','Theme','name',state.get('theme_override'))
            if not logout:refresh_surfaces()
        STATE.unlink()
        HOOK.unlink(missing_ok=True)
    if not logout:
        shader('restore');notify_palette()
    print('Restored the app palette and glass settings, preserving later manual changes.')
