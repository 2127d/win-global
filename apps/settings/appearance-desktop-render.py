#!/usr/bin/env python3
"""Render installed Plasma and Dolphin in a private compositor/configuration."""
import argparse
import configparser
import ctypes
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
CONFIG_FILES = ('kdeglobals', 'kwinrc', 'plasmarc', 'plasmashellrc',
                'plasma-org.kde.plasma.desktop-appletsrc', 'sequoiaelevenrc',
                'dolphinrc', 'kactivitymanagerdrc', 'kactivitymanagerdstaterc')

def config(path):
    result = configparser.ConfigParser(interpolation=None, strict=False, allow_no_value=True)
    result.optionxform = str
    result.read(path)
    return result

def set_entry(cfg, group, key, value):
    if not cfg.has_section(group): cfg.add_section(group)
    cfg.set(group, key, str(value))

def save(cfg, path):
    with path.open('w') as stream: cfg.write(stream, space_around_delimiters=False)

def prepare(root, args, source_config, source_data):
    for kind in ('config', 'data', 'cache', 'state', 'runtime'):
        (root/kind).mkdir(mode=0o700)
    for name in CONFIG_FILES:
        if (source_config/name).is_file(): shutil.copy2(source_config/name, root/'config'/name)
    # Only this display's containments, plus their nested tray containments.
    layout = config(root/'config/plasma-org.kde.plasma.desktop-appletsrc')
    keep = set()
    for section in layout.sections():
        match = re.fullmatch(r'Containments\]\[(\d+)', section)
        if match and layout.getint(section, 'lastScreen', fallback=-1) == args.screen:
            keep.add(match[1])
    for section in layout.sections():
        if any(section.startswith(f'Containments][{cid}][Applets]') for cid in keep):
            nested = layout.get(section, 'SystrayContainmentId', fallback='')
            if nested: keep.add(nested)
    for section in list(layout.sections()):
        match = re.match(r'Containments\]\[(\d+)(?:\]|$)', section)
        if match and match[1] not in keep: layout.remove_section(section)
    if not keep: raise RuntimeError('No desktop layout was found for this display.')
    for cid in keep:
        section = f'Containments][{cid}'
        if layout.getint(section, 'lastScreen', fallback=-1) >= 0:
            set_entry(layout, section, 'lastScreen', 0)
    save(layout, root/'config/plasma-org.kde.plasma.desktop-appletsrc')
    spec = importlib.util.spec_from_file_location('appearance_material', HERE/'appearance-material.py')
    material = importlib.util.module_from_spec(spec); spec.loader.exec_module(material)
    material.CONFIG, material.DATA = root/'config', source_data
    files = {}
    colors=config(root/'config/kdeglobals')
    kwin_source=config(root/'config/kwinrc')
    rgba=colors.get('Colors:Window','BackgroundNormal',fallback='242,243,245').split(',')
    translucent=len(rgba)>3 and int(rgba[3])<255
    active='solid' if not translucent else 'glass' if kwin_source.getboolean('Plugins',material.EFFECT+'Enabled',fallback=False) else 'transparent'
    # An unchanged selection must show existing custom colors/components;
    # staged choices use exactly the same recipe as the Apply action.
    changes=[] if args.material==active and args.mode=='current' else material.plan(args.material,args.mode)
    for entry in changes:
        name = entry['file']
        cfg = files.setdefault(name, config(root/'config'/name))
        set_entry(cfg, entry['group'], entry['key'], entry['after'])
    kwin = files.setdefault('kwinrc', config(root/'config/kwinrc'))
    # Capture a settled desktop without the session-entry animation.
    if kwin.has_section('Plugins'):
        for key in list(kwin['Plugins']):
            if key.startswith('sequoia-motion-'):
                set_entry(kwin, 'Plugins', key, 'false')
    set_entry(kwin, 'Windows', 'Placement', 'Centered')
    for name, cfg in files.items(): save(cfg, root/'config'/name)
    dolphin = config(root/'config/dolphinrc')
    set_entry(dolphin, 'General', 'RememberOpenedTabs', 'false')
    save(dolphin, root/'config/dolphinrc')

def inside(request_path):
    request = json.loads(Path(request_path).read_text())
    root = Path(request['root'])
    if (os.environ.get('XDG_CONFIG_HOME') != str(root/'config')
            or os.environ.get('WAYLAND_DISPLAY') != 'appearance-preview'
            or str(root/'runtime') not in os.environ.get('DBUS_SESSION_BUS_ADDRESS', '')):
        raise RuntimeError('Desktop renderer must run in its private session.')
    import dbus
    from PySide6.QtGui import QGuiApplication, QImage
    from PySide6.QtCore import QObject, Slot, ClassInfo
    from PySide6.QtDBus import QDBusConnection
    app = QGuiApplication([])
    bus = dbus.SessionBus()
    @ClassInfo(**{"D-Bus Interface":"org.sequoia.DesktopRenderer"})
    class ReadyProbe(QObject):
        payload = None
        @Slot(str)
        def ready(self, value): self.payload = json.loads(value)
    probe=ReadyProbe()
    qtbus=QDBusConnection.sessionBus()
    if not qtbus.registerObject('/DesktopRenderer',probe,QDBusConnection.ExportAllSlots):
        raise RuntimeError('Cannot verify the rendered application window.')
    children = []
    def launch(command):
        process = subprocess.Popen(command)
        children.append(process)
        return process
    def wait_service(name, timeout=15):
        end = time.monotonic()+timeout
        while time.monotonic()<end:
            if bus.name_has_owner(name): return
            time.sleep(.1)
        raise RuntimeError(f'{name} did not start in the desktop renderer.')
    try:
        launch(['/usr/libexec/kactivitymanagerd'])
        wait_service('org.kde.ActivityManager')
        launch(['/usr/bin/plasmashell', '--no-respawn', '--shell-plugin', 'org.kde.plasma.desktop'])
        wait_service('org.kde.plasmashell')
        shell = dbus.Interface(bus.get_object('org.kde.plasmashell','/PlasmaShell'), 'org.kde.PlasmaShell')
        # Render readiness is checked against actual desktop/panel objects.
        for attempt in range(80):
            try:
                inventory = json.loads(str(shell.evaluateScript('print(JSON.stringify({desktops:desktops().length,panels:panels().length}));')))
                if inventory['desktops'] > 0: break
            except (dbus.DBusException, ValueError): pass
            time.sleep(.1)
        else: raise RuntimeError('The cloned Plasma desktop did not become ready.')
        dolphin = launch(['/usr/bin/dolphin', '--new-window', str(Path.home())])
        time.sleep(3)
        width, height = request['width'], request['height']
        script = root/'place-app.js'
        script.write_text(f"function place(w) {{ if (w.pid === {dolphin.pid}) {{ w.frameGeometry = {{x:{round(width*.15)},y:{round(height*.16)},width:{round(width*.64)},height:{round(height*.70)}}}; workspace.activeWindow=w; callDBus('{qtbus.baseService()}','/DesktopRenderer','org.sequoia.DesktopRenderer','ready',JSON.stringify({{application:w.resourceClass,geometry:w.frameGeometry}})); }} }}; workspace.windowAdded.connect(place); for (const w of workspace.windowList()) place(w);")
        scripting = dbus.Interface(bus.get_object('org.kde.KWin','/Scripting'),'org.kde.kwin.Scripting')
        sid = bus.call_blocking('org.kde.KWin','/Scripting','org.kde.kwin.Scripting','loadScript','ss',(str(script),'appearance-sample-placement'))
        dbus.Interface(bus.get_object('org.kde.KWin',f'/Scripting/Script{sid}'),'org.kde.kwin.Script').run()
        for attempt in range(100):
            app.processEvents()
            if probe.payload: break
            time.sleep(.05)
        else: raise RuntimeError('The native Files window did not become ready.')
        time.sleep(2)
        effects = dbus.Interface(bus.get_object('org.kde.KWin','/Effects'),'org.freedesktop.DBus.Properties')
        loaded = list(effects.Get('org.kde.kwin.Effects','loadedEffects'))
        if request['material']=='glass' and 'sequoia_liquid_glass_v6' not in loaded:
            raise RuntimeError('Liquid Glass did not load in the renderer.')
        with tempfile.TemporaryFile() as stream:
            capture = dbus.Interface(bus.get_object('org.kde.KWin.ScreenShot2','/org/kde/KWin/ScreenShot2'),'org.kde.KWin.ScreenShot2')
            result = capture.CaptureWorkspace(dbus.Dictionary({'include-cursor':False},signature='sv'),dbus.types.UnixFd(stream.fileno()))
            size = int(result['height'])*int(result['stride'])
            for attempt in range(100):
                if os.fstat(stream.fileno()).st_size >= size: break
                time.sleep(.02)
            stream.seek(0); pixels=stream.read()
            if len(pixels)!=size: raise RuntimeError('Incomplete rendered desktop frame.')
            image=QImage(pixels,int(result['width']),int(result['height']),int(result['stride']),QImage.Format(int(result['format']))).copy()
            temporary=request['output']+'.new.png'
            if image.isNull() or not image.save(temporary): raise RuntimeError('Could not save rendered desktop.')
            os.replace(temporary,request['output'])
        Path(request['output']+'.json').write_text(json.dumps({'material':request['material'],'mode':request['mode'],'width':width,'height':height,'desktop':inventory,'renderer':'KWin + Plasma + Dolphin','effects':loaded,'application':probe.payload,'palette':QGuiApplication.palette().window().color().getRgb()}))
    finally:
        qtbus.unregisterObject('/DesktopRenderer')
        for process in reversed(children): process.terminate()
        for process in reversed(children):
            try: process.wait(timeout=3)
            except subprocess.TimeoutExpired: process.kill()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session')
    parser.add_argument('--material',choices=['solid','transparent','glass'])
    parser.add_argument('--mode',choices=['current','light','dark'])
    parser.add_argument('--output',type=Path)
    parser.add_argument('--screen',type=int,default=0)
    parser.add_argument('--width',type=int,default=1600)
    parser.add_argument('--height',type=int,default=900)
    args=parser.parse_args()
    if args.session: inside(args.session); return
    if not args.material or not args.mode or not args.output: parser.error('Material, mode and output are required.')
    if not (640<=args.width<=7680 and 480<=args.height<=4320): parser.error('Invalid preview dimensions.')
    args.output=args.output.resolve();args.output.parent.mkdir(parents=True,exist_ok=True)
    # Stop the private compositor if Settings is killed rather than closed.
    parent=os.getppid()
    ctypes.CDLL(None).prctl(1,signal.SIGTERM,0,0,0)
    if os.getppid()!=parent: raise SystemExit(1)
    source_config=Path(os.environ.get('XDG_CONFIG_HOME',Path.home()/'.config'))
    source_data=Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local/share'))
    with tempfile.TemporaryDirectory(prefix='appearance-desktop-') as temp:
        root=Path(temp)
        prepare(root,args,source_config,source_data)
        request=dict(root=str(root),output=str(args.output),material=args.material,mode=args.mode,width=args.width,height=args.height)
        (root/'request.json').write_text(json.dumps(request))
        env=dict(os.environ)
        for key in ('LD_PRELOAD','QT_STYLE_OVERRIDE','QML_IMPORT_PATH','QT_PLUGIN_PATH','QT_SCALE_FACTOR','QT_QUICK_BACKEND','SEQUOIA_APPEARANCE_HUB'):
            env.pop(key,None)
        for kind in ('CONFIG','DATA','CACHE','STATE','RUNTIME'): env[f'XDG_{kind}_HOME' if kind!='RUNTIME' else 'XDG_RUNTIME_DIR']=str(root/kind.lower())
        env.update(XDG_DATA_DIRS=str(source_data)+':'+os.environ.get('XDG_DATA_DIRS','/usr/local/share:/usr/share'),QT_QPA_PLATFORM='offscreen',QT_QPA_PLATFORMTHEME='kde',XDG_CURRENT_DESKTOP='KDE',KWIN_COMPOSE='O2',LIBGL_ALWAYS_SOFTWARE='1',KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1')
        launch=root/'session.sh'
        launch.write_text('#!/bin/sh\nexport QT_QPA_PLATFORM=wayland\nexec '+shlex.join([sys.executable,str(Path(__file__).resolve()),'--session',str(root/'request.json')])+'\n')
        launch.chmod(0o700)
        bus=root/'bus.conf'
        bus.write_text('<busconfig><type>session</type><listen>unix:tmpdir='+str(root/'runtime')+'</listen><policy context="default"><allow send_destination="*"/><allow receive_sender="*"/><allow own="*"/></policy></busconfig>')
        command=['dbus-run-session','--config-file='+str(bus),'--','kwin_wayland','--virtual','--width',str(args.width),'--height',str(args.height),'--inputmethod','/bin/true','--no-lockscreen','--no-global-shortcuts','--no-kactivities','--socket','appearance-preview','--exit-with-session',str(launch)]
        with Path(str(args.output)+'.log').open('w') as log:
            process=subprocess.Popen(command,env=env,stdout=log,stderr=log,start_new_session=True)
            def stop(signum=None, frame=None):
                try: os.killpg(process.pid,signal.SIGTERM)
                except ProcessLookupError: pass
                if signum is not None: raise SystemExit(128+signum)
            signal.signal(signal.SIGTERM,stop);signal.signal(signal.SIGINT,stop)
            try:
                process.wait(timeout=75)
                if process.returncode or not args.output.is_file(): raise RuntimeError('The isolated desktop renderer failed. See '+str(args.output)+'.log')
            finally:
                stop()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: os.killpg(process.pid,signal.SIGKILL);process.wait()
        print(json.dumps({'image':str(args.output),'material':args.material,'mode':args.mode}))

if __name__=='__main__': main()
