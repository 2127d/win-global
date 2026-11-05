#!/usr/bin/env python3
"""Run experimental liquid glass in a private KWin session, never the live compositor."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def stop(process):
    if process and process.poll() is None:
        process.terminate()
        try: process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir', type=Path, default=ROOT / '.staging/liquid-glass-build')
    p.add_argument('--output', type=Path, default=ROOT / 'preview/liquid-glass')
    p.add_argument('--headless', action='store_true', help='Capture and verify shader rendering using software OpenGL')
    p.add_argument('--discover', action='store_true', help='Preview the installed Discover theme against a live backdrop')
    p.add_argument('--child', action='store_true', help=argparse.SUPPRESS)
    args = p.parse_args()
    args.build_dir=args.build_dir.resolve(); args.output=args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=True)
    if args.child:
        compositor=None; fixture=None; app=None
        with (args.output/'kwin.log').open('w') as log, (args.output/'preview.log').open('w') as preview_log:
            try:
                backend=['--virtual'] if args.headless else ['--wayland-display',os.environ['GLASS_HOST_DISPLAY']]
                compositor=subprocess.Popen(['kwin_wayland',*backend,'--width','1200','--height','800',
                    '--no-lockscreen','--no-global-shortcuts','--no-kactivities','--inputmethod','/bin/true',
                    '--socket','sequoia-glass-test'],stdout=log,stderr=log)
                for _ in range(100):
                    if compositor.poll() is not None: raise RuntimeError('Private compositor exited; inspect kwin.log')
                    if (Path(os.environ['XDG_RUNTIME_DIR'])/'sequoia-glass-test').exists():break
                    time.sleep(.1)
                else:raise RuntimeError('Private compositor startup timed out')
                env=dict(os.environ,QT_QPA_PLATFORM='wayland',WAYLAND_DISPLAY='sequoia-glass-test')
                if args.headless:env['GLASS_CAPTURE']=str(args.output)
                if args.discover:env['GLASS_BACKDROP_ONLY']='1'
                fixture=subprocess.Popen([str(args.build_dir/'liquid-glass-preview')],env=env,stdout=preview_log,stderr=preview_log)
                if args.discover:
                    time.sleep(1)
                    env.pop('QT_STYLE_OVERRIDE',None);env.pop('QT_QPA_PLATFORMTHEME',None)
                    launcher=Path.home()/'.local/share/sequoia-eleven/quick-islands/sequoia-quick'
                    app=subprocess.Popen([str(launcher),'/usr/bin/plasma-discover'],env=env,stdout=preview_log,stderr=preview_log)
                    if args.headless:
                        if fixture.wait(timeout=35):raise RuntimeError('Discover capture failed')
                    else:
                        while compositor.poll() is None and app.poll() is None:time.sleep(.25)
                elif args.headless:
                    if fixture.wait(timeout=35):raise RuntimeError('Shader check failed; inspect preview.log')
                else:
                    while compositor.poll() is None and fixture.poll() is None:time.sleep(.25)
            finally:
                stop(app);stop(fixture);stop(compositor)
        return
    if not (args.build_dir/'liquid-glass-preview').exists():
        subprocess.run(['cmake','-S',str(ROOT/'desktop/effects/liquid-glass'),'-B',str(args.build_dir),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True)
        subprocess.run(['cmake','--build',str(args.build_dir),'--parallel','4'],check=True)
    host=os.environ.get('WAYLAND_DISPLAY')
    if not args.headless and not host: p.error('Interactive preview requires a Wayland session')
    with tempfile.TemporaryDirectory(prefix='sequoia-glass-',ignore_cleanup_errors=True) as temporary:
        base=Path(temporary); env=dict(os.environ)
        if host:env['GLASS_HOST_DISPLAY']=str(Path(os.environ['XDG_RUNTIME_DIR'])/host) if not host.startswith('/') else host
        for key in ('DISPLAY','WAYLAND_DISPLAY','LD_PRELOAD','QML_IMPORT_PATH','QML2_IMPORT_PATH','QT_QUICK_CONTROLS_STYLE'):
            env.pop(key,None)
        for kind in ('CONFIG','DATA','STATE','CACHE','RUNTIME'):
            directory=base/kind.lower();directory.mkdir(mode=0o700)
            env['XDG_'+kind+('_DIR' if kind=='RUNTIME' else '_HOME')]=str(directory)
        if args.discover:
            config=Path(os.environ.get('XDG_CONFIG_HOME',Path.home()/'.config'))/'kdeglobals'
            if config.exists():shutil.copy2(config,base/'config/kdeglobals')
            # The app wrapper and icons remain available from the existing user theme.
            env['XDG_DATA_DIRS']=str(Path.home()/'.local/share')+':/usr/local/share:/usr/share'
        else:env['XDG_DATA_DIRS']='/usr/local/share:/usr/share'
        (base/'config/kwinrc').write_text('[Plugins]\nsequoia_liquid_glassEnabled=true\nblurEnabled=false\ncontrastEnabled=false\nscaleEnabled=false\nfadingpopupsEnabled=false\n[Windows]\nPlacement=Centered\n[Effect-blur]\nBlurStrength=7\nNoiseStrength=0\nSaturation=110\n')
        env.update(QT_FORCE_STDERR_LOGGING='1',QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='generic',QT_STYLE_OVERRIDE='Fusion',
                   QT_PLUGIN_PATH=str(args.build_dir/'lib'),KWIN_COMPOSE='O2',KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1')
        if not args.discover:
            env['QT_QUICK_CONTROLS_STYLE']='Fusion'
            with (base/'config/kwinrc').open('a') as settings:settings.write('[Effect-sequoia_liquid_glass]\nFallbackCornerRadius=28\n')
        if args.headless:env['LIBGL_ALWAYS_SOFTWARE']='1'
        subprocess.run(['dbus-run-session','--',sys.executable,__file__,'--child','--build-dir',str(args.build_dir),'--output',str(args.output),
                        *(['--headless'] if args.headless else []),*(['--discover'] if args.discover else [])],env=env,check=True)

if __name__=='__main__':main()
