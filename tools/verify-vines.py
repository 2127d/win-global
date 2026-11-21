#!/usr/bin/env python3
"""Verify Jungle Vines inside an isolated compositor before live installation."""
import argparse, json, os, shlex, shutil, signal, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--stock-blur',action='store_true')
p.add_argument('--profile-only',action='store_true')
p.add_argument('--package',type=Path)
p.add_argument('--effect-id',default='sequoia-vines-v7')
a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=True)
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','Qt6Test','Qt6DBus'],text=True))
probe=a.output/'vines-probe'
subprocess.run(['c++','-fPIC',str(ROOT/'desktop/effects/vines/tests/probe.cpp'),str(ROOT/'desktop/effects/vines/motion.cpp'),'-I'+str(ROOT/'.staging/test-sdk'),'-o',str(probe),*flags,'-l:libei.so.1'],check=True)
with tempfile.TemporaryDirectory(prefix='gwenview-test-',ignore_cleanup_errors=True) as directory:
    base=Path(directory);env=dict(os.environ)
    for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'):
        path=base/kind.lower();path.mkdir(mode=0o700)
        env['XDG_RUNTIME_DIR' if kind=='RUNTIME' else f'XDG_{kind}_HOME']=str(path)
    for key in ('DISPLAY','WAYLAND_DISPLAY','QML2_IMPORT_PATH','QML_IMPORT_PATH','QT_PLUGIN_PATH','LD_PRELOAD'):
        env.pop(key,None)
    shutil.copytree(a.package or ROOT/'build/vines-v7/package',base/'data/kwin/effects'/a.effect_id)
    (base/'config/kwinrc').write_text('[Plugins]\nblurEnabled='+str(a.stock_blur).lower()+'\nsequoia_liquid_glass_v6Enabled='+str(not a.stock_blur).lower()+'\nfadingpopupsEnabled=false\nscaleEnabled=false\ncontrastEnabled=false\n')
    shutil.copy2(ROOT/'themes/color-schemes/SequoiaElevenDarkTransparent.colors',base/'config/kdeglobals')
    env.update(QT_QUICK_CONTROLS_STYLE='Fusion',QT_QPA_PLATFORM='offscreen',QT_STYLE_OVERRIDE='SequoiaElevenGlass',QT_PLUGIN_PATH=str(Path.home()/'.local/share/sequoia-eleven/downloads-style'),QT_QPA_PLATFORMTHEME='generic',QT_FORCE_STDERR_LOGGING='1',
               KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1',SEQUOIA_GWENVIEW_NATIVE_INPUT='1',VINES_CAPTURE=str(a.output),VINES_EFFECT_ID=a.effect_id,VINES_PROFILE_ONLY=str(int(a.profile_only)),SEQUOIA_VINES_PROFILE=str(int(a.profile_only)),
               KCOREADDONS_NO_XDG_DESKTOP_PORTAL='1',QT_NO_XDG_DESKTOP_PORTAL='1')
    launch=base/'launch.sh';launch.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexec '+shlex.quote(str(probe))+'\n');launch.chmod(0o755)
    command=['dbus-run-session','--','kwin_wayland','--virtual','--width','3440' if a.profile_only else '1600','--height','1000','--inputmethod','/bin/true',
             '--no-lockscreen','--no-global-shortcuts','--no-kactivities','--socket','gwenview-menu-test','--exit-with-session',str(launch)]
    logpath=a.output/'kwin.log'
    with logpath.open('w') as log:
        process=subprocess.Popen(command,env=env,stdout=log,stderr=log,start_new_session=True)
        try:process.wait(timeout=60)
        finally:
            try:os.killpg(process.pid,signal.SIGTERM)
            except ProcessLookupError:pass
            process.wait(timeout=5)
    text=logpath.read_text()
    assert ('PASS: pacing sample' if a.profile_only else 'PASS: full-width vines') in text and not any(s in text for s in ('FAIL:','ReferenceError','TypeError','is not a type','plugin cannot be loaded')),text
    print('\n'.join(line for line in text.splitlines() if line.startswith('PASS:')))

    if a.profile_only and not a.package:
        metrics=json.loads((a.output/'frame-metrics.json').read_text())
        presented=metrics['presented']
        assert 55 <= presented['hz'] <= 65 and presented['p95_ms'] <= 20, metrics
        print(f"PASS: 60 Hz virtual output presents {presented['hz']:.1f} distinct animation frames/s; p95 {presented['p95_ms']:.2f} ms")
