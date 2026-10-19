#!/usr/bin/env python3
"""Load and capture the Downloads fan in an isolated native Plasma popup."""
import os,subprocess,tempfile,signal,shlex,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Widgets','Qt6Test'],text=True))
subprocess.run(['c++','-std=c++17','-fPIC','-shared',str(root/'tools/verify-downloads-native.cpp'),'-o',str(root/'.staging/verify-downloads-native.so'),*flags,'-ldl'],check=True)
with tempfile.TemporaryDirectory(prefix='downloads-native-') as t:
 p=Path(t)
 package=p/'plasma/plasmoids/org.sequoia.downloads'
 shutil.copytree(root/'widgets/plasmoids/org.sequoia.downloads',package)
 main=package/'contents/ui/main.qml'
 # Native preview must never launch an external application, including when
 # user input happens to land on its briefly visible window.
 main.write_text(main.read_text().replace('Qt.openUrlExternally(destination)', 'console.log("TEST: intercepted activation")'))
 styles=p/'plugins/styles';styles.mkdir(parents=True)
 (styles/'sequoiaeleven6.so').symlink_to(root/'build/desktop/bin/sequoiaeleven6.so')
 (p/'icons').symlink_to(root/'themes/icons')
 (p/'plasma/desktoptheme').symlink_to(root/'themes/plasma/desktoptheme')
 folder=p/'downloads';folder.mkdir()
 for n in ['report.pdf','notes.txt','archive.zip']:(folder/n).touch()
 (p/'plasmawindowedrc').write_text('[Applets][42]\nplugin=org.sequoia.downloads\n[Applets][42][Configuration][General]\nfolderUrl='+folder.as_uri()+'\n[Applets][42][General]\nfolderUrl='+folder.as_uri()+'\n')
 env=dict(os.environ,XDG_CONFIG_HOME=t,XDG_DATA_HOME=t,XDG_STATE_HOME=t+'/state',XDG_CACHE_HOME=t+'/cache',DOWNLOADS_NATIVE_FOLDER=str(folder),QT_FORCE_STDERR_LOGGING='1',QT_QUICK_BACKEND='rhi',QT_PLUGIN_PATH=str(p/'plugins'),QT_STYLE_OVERRIDE='SequoiaEleven',DOWNLOADS_NATIVE_CAPTURE=str(root/'preview/downloads-native.png'))
 cmd=['dbus-run-session','--','env','LD_PRELOAD='+str(root/'.staging/verify-downloads-native.so'),'plasmawindowed','org.sequoia.downloads']
 with open(root/'preview/downloads-native.log','w') as log:
  proc=subprocess.Popen(cmd,env=env,stdout=log,stderr=log,start_new_session=True)
  try:code=proc.wait(timeout=15)
  finally:
   try:os.killpg(proc.pid,signal.SIGTERM)
   except ProcessLookupError:pass
 output=(root/'preview/downloads-native.log').read_text();print(output[-3500:]);raise SystemExit(code or (0 if 'PASS:' in output else 1))
