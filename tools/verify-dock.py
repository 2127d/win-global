#!/usr/bin/env python3
"""Verify native task delegates and hover in an isolated Wayland session."""
import os,subprocess,tempfile,signal,shlex,argparse
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
for flag in ('dark','large','rtl'): parser.add_argument('--'+flag,action='store_true')
args=parser.parse_args()
variant='-dark-large-rtl' if args.dark or args.large or args.rtl else ''
root=Path(__file__).resolve().parents[1]
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','Qt6Quick','Qt6Test'],text=True))
subprocess.run(['c++','-std=c++17','-shared','-fPIC',str(root/'tools/verify-dock.cpp'),'-o',str(root/'.staging/verify-dock.so'),*flags],check=True)
with tempfile.TemporaryDirectory(prefix='sequoia-dock-test-') as temp:
 p=Path(temp)
 (p/'plasmawindowedrc').write_text('[Applets][42]\nplugin=org.kde.plasma.icontasks\n[Applets][42][Configuration][General]\nlaunchers=applications:org.kde.dolphin.desktop,applications:org.kde.konsole.desktop,applications:org.kde.kwrite.desktop\niconSpacing=1\n')
 (p/'kdeglobals').write_text((root/'themes/color-schemes'/('SequoiaElevenDarkSolid.colors' if args.dark else 'SequoiaElevenLightSolid.colors')).read_text())
 (p/'color-schemes').symlink_to(root/'themes/color-schemes')
 (p/'plasmarc').write_text('[Theme]\nname=sequoia-eleven-dark\n')
 (p/'plasma').symlink_to(root/'themes/plasma');(p/'icons').symlink_to(root/'themes/icons')
 env=dict(os.environ,XDG_CONFIG_HOME=temp,XDG_DATA_HOME=temp,XDG_CACHE_HOME=temp+'/cache',XDG_STATE_HOME=temp+'/state',QT_QPA_PLATFORM='wayland',QT_QUICK_BACKEND='software',QT_FORCE_STDERR_LOGGING='1',SEQUOIA_DOCK_TEST=str(root/('preview/dock-native'+variant+'.png')))
 for flag in ('dark','large','rtl'):
  if getattr(args,flag): env['SEQUOIA_VERIFY_'+flag.upper()]='1'
 cmd=['dbus-run-session','--','env','LD_PRELOAD='+str(root/'.staging/verify-dock.so'),'python3',str(root/'.staging/control-center-data/sequoia-eleven/control-center/launch.py'),'plasmawindowed','org.kde.plasma.icontasks']
 with open(root/('preview/dock-native'+variant+'.log'),'w') as log:
  proc=subprocess.Popen(cmd,env=env,stdout=log,stderr=log,start_new_session=True)
  try: code=proc.wait(timeout=20)
  finally:
   try:os.killpg(proc.pid,signal.SIGTERM)
   except ProcessLookupError:pass
 output=(root/('preview/dock-native'+variant+'.log')).read_text();print(output[-4500:]);raise SystemExit(code or (0 if 'PASS: native task delegates' in output else 1))
