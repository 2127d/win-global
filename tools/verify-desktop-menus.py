#!/usr/bin/env python3
"""Check real desktop menu cascades with private Plasma/KWin and real input."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--plugins', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--motion-script', type=Path, help='Exercise popup animation frames using this effect script')
p.add_argument('--glass', action='store_true', help='Use Liquid Glass instead of stock Blur')
a = p.parse_args(); a.output.mkdir(parents=True, exist_ok=True)
probe = a.output.resolve() / 'desktop-menu-check.so'
flags = shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Widgets','Qt6Test','Qt6DBus'],text=True))
flags += ['-I'+str(ROOT/'.staging/test-sdk'),'-l:libei.so.1']
subprocess.run(['c++','-shared','-fPIC',str(ROOT/'tools/test-support/desktop-menu-spacing.cpp'),'-o',str(probe),*flags],check=True)
with tempfile.TemporaryDirectory(prefix='gwenview-test-', ignore_cleanup_errors=True) as temp:
    base=Path(temp);env=dict(os.environ)
    for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'):
        path=base/kind.lower();path.mkdir(mode=0o700)
        env['XDG_RUNTIME_DIR' if kind=='RUNTIME' else f'XDG_{kind}_HOME']=str(path)
    for key in ('DISPLAY','WAYLAND_DISPLAY','QML2_IMPORT_PATH','QML_IMPORT_PATH','LD_PRELOAD'):
        env.pop(key,None)
    text=(ROOT/'themes/color-schemes/SequoiaElevenDarkTransparent.colors').read_text()
    (base/'config/kdeglobals').write_text(text+'\n[Icons]\nTheme=breeze-dark\n')
    kwin = '[Plugins]\nblurEnabled=' + ('false' if a.glass else 'true') + '\nsequoia_liquid_glass_v6Enabled=' + ('true' if a.glass else 'false') + '\n'
    if a.motion_script:
        from motion_palette import effect_sources
        package = base/'data/kwin/effects/sequoia-motion-fadingpopups'
        shutil.copytree(effect_sources()/'fadingpopups', package)
        import json
        metadata = json.loads((package/'metadata.json').read_text())
        metadata['KPlugin']['Id'] = 'sequoia-motion-fadingpopups'
        (package/'metadata.json').write_text(json.dumps(metadata))
        shutil.copy2(a.motion_script, package/'contents/code/main.js')
        kwin += 'sequoia-motion-fadingpopupsEnabled=true\nfadingpopupsEnabled=false\n'
        with (base/'config/kdeglobals').open('a') as config:
            config.write('[KDE]\nAnimationDurationFactor=10\n')
        env['SEQUOIA_DESKTOP_MENU_MOTION_CHECK'] = '1'
    (base/'config/kwinrc').write_text(kwin)
    env.update(QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='kde',QT_STYLE_OVERRIDE='SequoiaElevenGlass' if a.glass else 'SequoiaEleven',
        QT_PLUGIN_PATH=str(a.plugins.resolve()), QT_FORCE_STDERR_LOGGING='1', KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1',
        KCOREADDONS_NO_XDG_DESKTOP_PORTAL='1', QT_NO_XDG_DESKTOP_PORTAL='1', SEQUOIA_GWENVIEW_NATIVE_INPUT='1',
        SEQUOIA_DESKTOP_MENU_CHECK='1', SEQUOIA_DESKTOP_MENU_CAPTURE=str(a.output.resolve()/'desktop-'))
    launch=base/'launch.sh'
    launch.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexport LD_PRELOAD='+shlex.quote(str(probe))+'\nexec plasmashell --no-respawn\n');launch.chmod(0o755)
    command=['dbus-run-session','--','kwin_wayland','--virtual','--width','1600','--height','1200','--inputmethod','/bin/true','--no-lockscreen','--no-global-shortcuts','--no-kactivities','--socket','gwenview-menu-test','--exit-with-session',str(launch)]
    logpath=a.output/'desktop.log'
    with logpath.open('w') as log:
        process=subprocess.Popen(command,env=env,stdout=log,stderr=log,start_new_session=True)
        try:process.wait(timeout=40)
        finally:
            try:os.killpg(process.pid,signal.SIGTERM)
            except ProcessLookupError:pass
            process.wait(timeout=5)
    log=logpath.read_text()
    assert 'PASS: actual Plasma desktop' in log and 'FAIL:' not in log,log
    print('\n'.join(line for line in log.splitlines() if line.startswith(('PASS:','DESKTOP GAP:','POPUP FRAME:'))))
