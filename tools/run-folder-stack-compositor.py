#!/usr/bin/env python3
"""Run a widget check on an isolated virtual Wayland display."""
import os,subprocess,sys,time
from pathlib import Path
env=dict(os.environ)
runtime=Path(env['XDG_RUNTIME_DIR'])
assert runtime.parent.name.startswith('folder-stack-')
compositor_env=dict(env,QT_QPA_PLATFORM='offscreen')
for key in ('LD_PRELOAD','WAYLAND_DISPLAY','DISPLAY'):compositor_env.pop(key,None)
with (runtime.parent/'compositor.log').open('w') as log:
 compositor=subprocess.Popen(['kwin_wayland','--virtual','--width','1400','--height','1000','--no-lockscreen','--no-global-shortcuts','--no-kactivities','--socket','stack-test'],env=compositor_env,stdout=log,stderr=log)
 try:
  for _ in range(100):
   if (runtime/'stack-test').exists():break
   if compositor.poll() is not None:raise RuntimeError('Private compositor exited')
   time.sleep(.05)
  else:raise RuntimeError('Private compositor did not start')
  env.update(QT_QPA_PLATFORM='wayland',WAYLAND_DISPLAY='stack-test',QT_QUICK_BACKEND='rhi')
  result=subprocess.run(sys.argv[1:],env=env)
 finally:
  compositor.terminate()
  try:compositor.wait(timeout=5)
  except subprocess.TimeoutExpired:compositor.kill();compositor.wait()
sys.exit(result.returncode)
