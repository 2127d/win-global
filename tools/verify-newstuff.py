#!/usr/bin/env python3
"""Exercise the shared NewStuff UI without a store connection or live theme writes."""
import argparse,os,shlex,subprocess,tempfile,shutil,signal
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dark',action='store_true');p.add_argument('--glass',action='store_true');p.add_argument('--minimal',action='store_true');p.add_argument('--composited',action='store_true');p.add_argument('--native',choices=['dialog','plasma']);args=p.parse_args()
binary=ROOT/'.staging/verify-newstuff'
flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6Quick','Qt6Widgets','Qt6Test','Qt6QuickControls2'],text=True))
subprocess.run(['c++','-std=c++17','-fPIC',str(ROOT/'tools/verify-newstuff.cpp'),'-I/usr/include/KF6/KI18n','-o',str(binary),*flags,'-lKF6I18n','-lKF6I18nQml'],check=True)
with tempfile.TemporaryDirectory(prefix='sequoia-newstuff-') as temporary:
    t=Path(temporary)
    for name in ('config','data','cache','state','runtime'):(t/name).mkdir(mode=0o700)
    (t/'payload.txt').write_text('Local fixture only.\n')
    (t/'providers.xml').write_text(f'<knewstuffproviders><provider downloadurl="{(t/"entries.xml").as_uri()}" nouploadurl="https://example.invalid/"><title>Local fixture</title></provider></knewstuffproviders>')
    preview=(ROOT/'preview/settings-hub-dark-composited/plasma-style-client.png').as_uri()
    (t/'entries.xml').write_text('<knewstuff>'+''.join(f'<stuff><name>Sequoia Sample {i}</name><author>Local fixture</author><summary>Shared theme browser preview.</summary><version>1.0</version><release>2026-11-13</release><licence>MIT</licence><id>sample{i}</id><preview>{preview}</preview><previewbig>{preview}</previewbig><payload>{(t/"payload.txt").as_uri()}</payload></stuff>' for i in range(1,4))+'</knewstuff>')
    (t/'catalog.knsrc').write_text(f'[KNewStuff]\nName=Plasma Styles\nProvidersUrl={(t/"providers.xml").as_uri()}\nTargetDir=sequoia-test-only\nUncompress=never\n')
    (t/'test.qml').write_text('import QtQuick\nimport org.kde.newstuff as N\nN.Dialog { property int installedStatus: N.Entry.Installed; property int deletedStatus: N.Entry.Deleted; configFile: "'+str(t/'catalog.knsrc')+'"; width: 1100; height: 800; visible: true; viewMode: N.Page.Preview }\n')
    # Theme watches the isolated kdeglobals; test every requested material via it.
    material='glass' if args.glass else 'solid'
    (t/'config/kdeglobals').write_text((ROOT / ('themes/color-schemes/SequoiaEleven'+('Dark' if args.dark else 'Light')+('Transparent' if args.glass else 'Solid')+'.colors')).read_text() + '\n[KDE]\nwidgetStyle='+('SequoiaElevenGlass' if args.glass else 'SequoiaEleven')+'\n[Icons]\nTheme=breeze\n')
    if args.glass: (t/'config/kwinrc').write_text('[Plugins]\nsequoia_liquid_glass_v6Enabled=true\n')
    env=dict(os.environ,QT_FORCE_STDERR_LOGGING='1',QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='generic',QT_QUICK_BACKEND='software',QT_QUICK_CONTROLS_STYLE='org.kde.desktop',QT_STYLE_OVERRIDE='SequoiaElevenGlass' if args.glass else 'SequoiaEleven',QT_PLUGIN_PATH=str(ROOT/'.staging/desktop/usr/lib64/qt6/plugins'),QML_IMPORT_PATH=str(ROOT/('.staging/newstuff/qml' if args.minimal else '.staging/quick-islands')),XDG_CONFIG_HOME=str(t/'config'),XDG_DATA_HOME=str(t/'data'),XDG_STATE_HOME=str(t/'state'),XDG_CACHE_HOME=str(t/'cache'),XDG_RUNTIME_DIR=str(t/'runtime'))
    if args.dark:env['TEST_DARK']='1'
    if args.glass:env['TEST_GLASS']='1'
    env.pop('LD_PRELOAD',None)
    capture=ROOT/'preview'/('newstuff-'+material+('-dark' if args.dark else '-light')+('-minimal' if args.minimal else '')+'.png')
    application=[str(binary),str(t/'test.qml'),str(capture)]
    if args.native:
        hook=ROOT/'.staging/verify-newstuff-native.so'
        native_flags=shlex.split(subprocess.check_output(['pkg-config','--cflags','--libs','Qt6QuickWidgets','Qt6Quick','Qt6Widgets','Qt6Test'],text=True))
        subprocess.run(['c++','-std=c++17','-fPIC','-shared',str(ROOT/'tools/verify-newstuff-native.cpp'),'-o',str(hook),*native_flags],check=True)
        shared=t/'data/sequoia-eleven';shared.mkdir(parents=True)
        (shared/'newstuff').symlink_to(ROOT/'.staging/newstuff',target_is_directory=True)
        (shared/'quick-islands').mkdir()
        (shared/'quick-islands/qml').symlink_to(ROOT/'.staging/quick-islands',target_is_directory=True)
        env.pop('QML_IMPORT_PATH',None);env.pop('QML2_IMPORT_PATH',None)
        env['NEWSTUFF_NATIVE_CAPTURE']=str(capture)
        if args.native=='plasma':
            (t/'data/knsrcfiles').mkdir()
            shutil.copy2(t/'catalog.knsrc',t/'data/knsrcfiles/plasma-themes.knsrc')
            env.update(NEWSTUFF_FROM_PLASMA='1',SEQUOIA_APPEARANCE_HUB='1',SEQUOIA_DESKTOP_RENDERING='0')
            app=[str(ROOT/'.staging/settings-hub-build/bin/systemsettings'),'kcm_desktoptheme']
        else: app=['knewstuff-dialog6',str(t/'catalog.knsrc')]
        application=['env','LD_PRELOAD='+str(hook),*app]
    command=['dbus-run-session','--',*application]
    if args.composited:
        env.update(QT_QUICK_BACKEND='rhi',QSG_RHI_BACKEND='opengl',LIBGL_ALWAYS_SOFTWARE='1',KWIN_COMPOSE='O2')
        launcher=t/'launch.sh'
        launcher.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexec '+shlex.join(application)+'\n');launcher.chmod(0o700)
        command=['dbus-run-session','--','kwin_wayland','--virtual','--width','1300','--height','1000','--inputmethod','/bin/true','--no-lockscreen','--no-global-shortcuts','--no-kactivities','--socket','newstuff-test','--exit-with-session',str(launcher)]
    log=ROOT/'preview/newstuff-verification.log'
    with log.open('w') as output:
        proc=subprocess.Popen(command,env=env,stdout=output,stderr=output,start_new_session=True)
        try: code=proc.wait(timeout=35)
        finally:
            try: os.killpg(proc.pid,signal.SIGTERM)
            except ProcessLookupError: pass
    result=log.read_text();print(result[-10000:])
    if code or 'PASS:' not in result: raise SystemExit(code or 1)
