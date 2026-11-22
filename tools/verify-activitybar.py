#!/usr/bin/env python3
"""Check Activity Bar in a private Wayland and D-Bus session; keep logs in /tmp."""
import os,sys,subprocess,shlex,shutil,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];WORK=ROOT/'.staging/activitybar-test';WORK.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(ROOT/'tools'))
import shell_overrides
if len(sys.argv)>1 and sys.argv[1]=='child':
 base=Path(sys.argv[2]);log=(base/'plasma.log').open('w');kwin=None;shell=None
 try:
  kwin=subprocess.Popen(['kwin_wayland','--virtual','--width','1280','--height','720','--no-lockscreen','--no-global-shortcuts','--socket','activity-test'],stdout=log,stderr=log)
  for _ in range(100):
   if (base/'runtime/activity-test').exists():break
   time.sleep(.1)
  env=dict(os.environ,QT_QPA_PLATFORM='wayland',WAYLAND_DISPLAY='activity-test',LD_PRELOAD=str(WORK/'probe.so'),SEQUOIA_PICKER_READY=str(base/'ready'),ACTIVITY_CAPTURE=str(base/'capture'))
  shell=subprocess.Popen(['plasmashell','--no-respawn'],env=env,stdout=log,stderr=log)
  for _ in range(100):
   r=subprocess.run(['qdbus-qt6','org.kde.plasmashell','/PlasmaShell','org.kde.PlasmaShell.evaluateScript','print(desktops().length)'],capture_output=True,text=True)
   if r.returncode==0 and r.stdout.strip()=='1':break
   time.sleep(.1)
  subprocess.run(['qdbus-qt6','org.kde.plasmashell','/PlasmaShell','org.kde.PlasmaShell.evaluateScript','panels().forEach(p=>p.remove()); var p=new Panel; p.location="bottom"; p.height=40; p.addWidget("org.sequoia.activitybar")'],capture_output=True,check=True)
  (base/'ready').touch();sys.exit(shell.wait(timeout=30))
 finally:
  for p in (shell,kwin):
   if p and p.poll() is None:
    p.terminate()
    try:p.wait(timeout=5)
    except subprocess.TimeoutExpired:p.kill();p.wait()
else:
 base=Path('/tmp')/('sq-act-'+str(time.time_ns()));base.mkdir()
 flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Widgets','Qt6Test','Qt6DBus'],text=True));subprocess.run(['c++','-std=c++17','-fPIC','-shared',str(ROOT/'tools/verify-activitybar.cpp'),'-o',str(WORK/'probe.so'),*flags],check=True)
 env=dict(os.environ,QT_QPA_PLATFORM='offscreen',QSG_RHI_BACKEND='opengl',LIBGL_ALWAYS_SOFTWARE='1',QT_QPA_PLATFORMTHEME='kde',QT_STYLE_OVERRIDE='Fusion',KWIN_COMPOSE='O2',QT_FORCE_STDERR_LOGGING='1')
 for key in ('DISPLAY','WAYLAND_DISPLAY','LD_PRELOAD','QML_IMPORT_PATH','QT_PLUGIN_PATH'):env.pop(key,None)
 for kind in ('CONFIG','DATA','STATE','CACHE','RUNTIME'):
  path=base/kind.lower();path.mkdir(mode=0o700);env[f'XDG_{kind}_'+('DIR' if kind=='RUNTIME' else 'HOME')]=str(path)
 env['XDG_DATA_DIRS']=str(Path.home()/'.local/share')+':/usr/local/share:/usr/share';env['XDG_CONFIG_DIRS']='/etc/xdg';env['AKONADI_INSTANCE']=base.name
 shell_overrides.stage(base/'data',base/'unused',{'plasma-style'},{})
 shutil.copytree(ROOT/'widgets/plasmoids/org.sequoia.activitybar',base/'data/plasma/plasmoids/org.sequoia.activitybar')
 (base/'config/plasmarc').write_text('[Theme]\nname=sequoia-eleven-light\n');(base/'config/plasma-welcomerc').write_text('[General]\nLastSeenVersion=999.0\n')
 shutil.copy2(ROOT/'themes/color-schemes/SequoiaElevenLightSolid.colors',base/'config/kdeglobals')
 with (base/'session.log').open('w') as log:r=subprocess.run(['dbus-run-session','--',sys.executable,__file__,'child',str(base)],env=env,stdout=log,stderr=log,timeout=60)
 print(base);print('\n'.join(l for l in (base/'plasma.log').read_text().splitlines() if any(x in l for x in ('PASS','FAIL','GEOMETRY','Error','Activity','activitybar','Binding loop'))));sys.exit(r.returncode)
