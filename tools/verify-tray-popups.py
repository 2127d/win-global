#!/usr/bin/env python3
"""Test tray focus and popup count in a private panel, with optional candidate archives."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from widget_payload import module, NAMES

ROOT = Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--folder',type=Path)
parser.add_argument('--child',type=Path,help=argparse.SUPPRESS)
args=parser.parse_args()
if args.child:
    base=args.child
    compositor=shell=None
    try:
        with (base/'kwin.log').open('w') as log:
            compositor=subprocess.Popen(['kwin_wayland','--virtual','--width','1280','--height','800',
                '--no-lockscreen','--no-global-shortcuts','--socket','tray-test'],stdout=log,stderr=log)
        for _ in range(100):
            if (base/'run/tray-test').exists(): break
            if compositor.poll() is not None: raise RuntimeError((base/'kwin.log').read_text())
            time.sleep(.1)
        env=dict(os.environ,QT_QPA_PLATFORM='wayland',WAYLAND_DISPLAY='tray-test',
                 LD_PRELOAD=str(base/'probe.so'),SEQUOIA_TRAY_POPUP_TEST='1')
        with (base/'shell.log').open('w') as log:
            shell=subprocess.Popen(['plasmashell','--no-respawn'],env=env,stdout=log,stderr=log)
            result=shell.wait(timeout=30)
        output=(base/'shell.log').read_text()
        print(output[-14000:],flush=True)
        if result or 'PASS: repeated single-click' not in output: raise SystemExit(1)
    finally:
        for p in (shell,compositor):
            if p and p.poll() is None:
                p.terminate()
                try: p.wait(timeout=4)
                except subprocess.TimeoutExpired: p.kill();p.wait()
    raise SystemExit(0)

with tempfile.TemporaryDirectory(prefix='sequoia-tray-popup-',ignore_cleanup_errors=True) as temporary:
    base=Path(temporary)
    for name in ('data','config','cache','state','run','services'): (base/name).mkdir(mode=0o700)
    (base/'data/plasma').mkdir()
    (base/'data/plasma/desktoptheme').symlink_to(ROOT/'plasma/desktoptheme', target_is_directory=True)
    plugin_dir=module('install-widget-archives').plugin_directory()
    if args.folder:
        installer=module('install-widget-archives')
        packages=installer.unpack_archives(args.folder.resolve(),base/'unpack')
        installer.install_files(packages,base/'system')
    else:
        for name in NAMES:
            source=Path('/usr/share/plasma/plasmoids')/name
            if source.exists(): shutil.copytree(source,base/'system/usr/share/plasma/plasmoids'/name)
            source=plugin_dir/(name+'.so')
            if source.exists():
                dest=base/'system'/source.relative_to('/');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
    known=','.join([p.stem for p in plugin_dir.glob('*.so')]+list(NAMES))
    enabled=','.join('org.sequoia.'+n for n in ('volume','notifications','clipboard'))
    (base/'config/plasma-org.kde.plasma.desktop-appletsrc').write_text(
        '[Containments][1]\nplugin=org.kde.panel\nlocation=3\nformfactor=2\nlastScreen=0\n'
        '[Containments][1][General]\nAppletOrder=2\n'
        '[Containments][1][Applets][2]\nplugin=org.sequoia.systemtray\n'
        '[Containments][1][Applets][2][General]\nknownItems='+known+'\nextraItems='+enabled+'\nshowAllItems=true\npin=false\n')
    (base/'config/plasmashellrc').write_text('[PlasmaViews][Panel 1]\nlengthMode=0\n[PlasmaViews][Panel 1][Defaults]\nthickness=40\n')
    (base/'config/kdeglobals').write_text('[KDE]\nwidgetStyle=Breeze\n[Icons]\nTheme=breeze\n')
    flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Widgets','Qt6Test'],text=True))
    subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(ROOT/'tools/verify-tray-popups.cpp'),'-o',str(base/'probe.so'),*flags],check=True)
    shutil.copy2('/usr/share/dbus-1/services/org.kde.ActivityManager.service',base/'services')
    bus=base/'bus.conf'
    bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir='+str(base)+'</listen><servicedir>'+str(base/'services')+'</servicedir><policy context="default"><allow send_destination="*"/><allow receive_sender="*"/><allow own="*"/></policy></busconfig>')
    env=dict(os.environ,HOME=str(base),XDG_CONFIG_HOME=str(base/'config'),XDG_DATA_HOME=str(base/'data'),
        XDG_CACHE_HOME=str(base/'cache'),XDG_STATE_HOME=str(base/'state'),XDG_RUNTIME_DIR=str(base/'run'),
        XDG_DATA_DIRS=str(base/'system/usr/share')+':/usr/share',
        QT_PLUGIN_PATH=str(base/'system'/plugin_dir.relative_to('/').parent.parent),
        QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='Fusion',
        QT_QPA_PLATFORMTHEME='generic',QT_FORCE_STDERR_LOGGING='1',KWIN_COMPOSE='Q')
    for key in ('DISPLAY','WAYLAND_DISPLAY','LD_PRELOAD'): env.pop(key,None)
    log=ROOT/'.staging/tray-popup-test.log'
    with log.open('w') as output:
        proc=subprocess.Popen(['dbus-run-session','--config-file='+str(bus),'--',sys.executable,str(Path(__file__).resolve()),'--child',str(base)],env=env,stdout=output,stderr=output,start_new_session=True)
        try: result=proc.wait(timeout=45)
        finally:
            try: os.killpg(proc.pid,signal.SIGTERM)
            except ProcessLookupError: pass
            proc.wait()
    print(log.read_text()[-14000:])
    raise SystemExit(result)
