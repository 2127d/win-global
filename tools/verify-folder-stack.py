#!/usr/bin/env python3
"""Exercise Folder Stack in a private Plasma/DBus session and temporary folder."""
import os, subprocess, tempfile, signal, shlex, shutil, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
helper=root/'.staging/verify-folder-stack.so'
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Widgets','Qt6Test'],text=True))
subprocess.run(['c++','-std=c++17','-fPIC','-shared',str(root/'tools/verify-folder-stack.cpp'),'-o',str(helper),*flags],check=True)
with tempfile.TemporaryDirectory(prefix='folder-stack-') as t:
 p=Path(t);package=p/'plasma/plasmoids/org.sequoia.folderstack'
 shutil.copytree(root/'widgets/plasmoids/org.sequoia.folderstack',package)
 runtime=p/'runtime';runtime.mkdir(mode=0o700)
 (p/'icons').symlink_to(root/'themes/icons')
 (p/'plasma/desktoptheme').symlink_to(root/'themes/plasma/desktoptheme')
 folder=p/'files';folder.mkdir()
 for n in ['report.pdf','notes.txt','archive.zip']:(folder/n).touch()
 env=dict(os.environ,XDG_CONFIG_HOME=t,XDG_DATA_HOME=t,XDG_RUNTIME_DIR=str(runtime),KCOREADDONS_NO_XDG_DESKTOP_PORTAL='1',QT_NO_XDG_DESKTOP_PORTAL='1',XDG_STATE_HOME=t+'/state',XDG_CACHE_HOME=t+'/cache',QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QT_STYLE_OVERRIDE='Fusion',QT_FORCE_STDERR_LOGGING='1',STACK_FOLDER=str(folder),STACK_CAPTURE=str(root/'preview/folder-stack.png'))
 cmd=['env','LD_PRELOAD='+str(helper),'plasmawindowed','org.sequoia.folderstack']
 if '--native' in sys.argv:cmd=['python3',str(root/'tools/run-folder-stack-compositor.py'),*cmd]
 cmd=['dbus-run-session','--',*cmd]
 with open(root/'preview/folder-stack.log','w') as log:
  proc=subprocess.Popen(cmd,env=env,stdout=log,stderr=log,start_new_session=True)
  try:code=proc.wait(timeout=25)
  finally:
   try:os.killpg(proc.pid,signal.SIGTERM)
   except ProcessLookupError:pass
 output=(root/'preview/folder-stack.log').read_text();print('\n'.join(line for line in output.splitlines() if any(key in line for key in ['FAIL:', 'PASS:', 'Error', 'file:///'])));raise SystemExit(code or (0 if 'PASS:' in output else 1))
