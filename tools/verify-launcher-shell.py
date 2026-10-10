#!/usr/bin/env python3
"""Exercise launcher selection in a private Plasma/Wayland shell with temporary settings."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

import shell_overrides as override

ROOT = Path(__file__).resolve().parents[1]
PLUGINS = {'org.kde.plasma.kickoff': True, 'org.kde.plasma.digitalclock': False}


def stop(process):
    if process and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def child(base):
    assert os.environ.get('SEQUOIA_LAUNCHER_TEST') == str(base)
    compositor = shell = None
    log = (base / 'plasma.log').open('w')
    try:
        compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '900',
                                      '--no-lockscreen', '--no-global-shortcuts', '--socket', 'sequoia-launcher-test'], stdout=log, stderr=log)
        socket = base / 'runtime/sequoia-launcher-test'
        for _ in range(100):
            if socket.exists(): break
            assert compositor.poll() is None, 'Private compositor exited'
            time.sleep(.1)
        assert socket.exists(), 'Private compositor did not start'
        os.environ.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='sequoia-launcher-test')
        shell = subprocess.Popen(['plasmashell', '--no-respawn'], stdout=log, stderr=log)
        for _ in range(100):
            ready = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript', 'print(desktops().length);'], capture_output=True, text=True, timeout=10)
            if ready.returncode == 0 and ready.stdout.strip() == '1': break
            time.sleep(.1)
        else:
            raise RuntimeError('Private desktop did not become ready')
        script = '''panels().forEach(p => p.remove());
var p = new Panel; p.location = "bottom"; p.height = 70; p.screen = 0;
p.addWidget("org.kde.plasma.kickoff");
p.addWidget("org.kde.plasma.digitalclock");
p.addWidget("org.kde.plasma.systemtray");
print("created");'''
        for _ in range(100):
            result = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript', script], capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and 'created' in result.stdout: break
            assert shell.poll() is None, 'Private shell exited'
            time.sleep(.1)
        else:
            raise RuntimeError('Private shell did not start')
        subprocess.run(['qdbus-qt6','org.kde.plasmashell','/PlasmaShell','org.kde.PlasmaShell.activateLauncherMenu'], check=True, capture_output=True)
        widgets = subprocess.run(['qdbus-qt6','org.kde.plasmashell','/PlasmaShell','org.kde.PlasmaShell.evaluateScript', 'print(JSON.stringify(panels()[0].widgets().map(w => w.type)));'], check=True, capture_output=True,text=True)
        assert 'org.kde.plasma.systemtray' in json.loads(widgets.stdout), widgets.stdout
        seen = {}
        for _ in range(100):
            output = (base / 'plasma.log').read_text()
            for line in output.splitlines():
                if 'SEQUOIA_FEEDBACK ' in line:
                    entry = json.loads(line.split('SEQUOIA_FEEDBACK ', 1)[1])
                    if entry['plugin'] in PLUGINS and entry['width'] > 0 and entry['height'] > 0:
                        seen[entry['plugin']] = entry
            if all(plugin in seen and seen[plugin]['square'] == square for plugin,square in PLUGINS.items()): break
            time.sleep(.1)
        assert set(seen) == set(PLUGINS), seen
        for plugin, square in PLUGINS.items():
            entry = seen[plugin]
            assert entry['square'] == square, entry
            if square:
                assert entry['prefix'] == ['sequoia-launcher'] and entry['width'] == entry['height'], entry
        assert not any('CompactApplet.qml' in line and any(error in line for error in
                       ('Error:', 'is not defined', 'Cannot assign', 'unavailable')) for line in output.splitlines()), output
        print('PASS: real Plasma selects square launcher feedback and standard calendar feedback, loads the system tray, and has no CompactApplet errors.', flush=True)
    finally:
        stop(shell)
        subprocess.run(['akonadictl','--instance',os.environ['AKONADI_INSTANCE'],'--wait','stop'], stdout=log,stderr=log,timeout=20,check=False)
        stop(compositor)
        log.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--child', type=Path)
    args=parser.parse_args()
    if args.child:
        child(args.child)
        return
    with tempfile.TemporaryDirectory(prefix='sequoia-launcher-native-', ignore_cleanup_errors=True) as temporary:
        base=Path(temporary)
        env=dict(os.environ, SEQUOIA_LAUNCHER_TEST=str(base), QT_QPA_PLATFORM='offscreen',
                 QT_QUICK_BACKEND='software', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                 KWIN_COMPOSE='Q', QT_FORCE_STDERR_LOGGING='1')
        for key in ('DISPLAY','WAYLAND_DISPLAY','LD_PRELOAD','QT_PLUGIN_PATH','QML_IMPORT_PATH'):
            env.pop(key,None)
        for kind in ('CONFIG','DATA','STATE','CACHE','RUNTIME'):
            path=base/kind.lower(); path.mkdir(mode=0o700)
            env['XDG_'+kind+('_DIR' if kind=='RUNTIME' else '_HOME')]=str(path)
        env['XDG_DATA_DIRS']='/usr/local/share:/usr/share'
        env['XDG_CONFIG_DIRS']='/etc/xdg'
        env['AKONADI_INSTANCE']=base.name
        override.stage(base/'data', base/'unused', {'plasma-style'}, {})
        for mode in ('light','dark'):
            source=ROOT/f'plasma/desktoptheme/sequoia-eleven-{mode}'
            shutil.copytree(source,base/'data/plasma/desktoptheme'/source.name)
        (base/'config/plasmarc').write_text('[Theme]\nname=sequoia-eleven-dark\n')
        (base/'config/plasma-welcomerc').write_text('[General]\nLastSeenVersion=999.0\n')
        target=base/'data'/override.SHELL/override.COMPACT
        probe='''        Timer {
            interval: 250; repeat: true; running: true
            onTriggered: console.warn("SEQUOIA_FEEDBACK " + JSON.stringify({
                plugin: Plasmoid.pluginName, square: expandedItem.squareLauncher,
                prefix: expandedItem.prefix, width: expandedItem.width, height: expandedItem.height
            }))
        }
'''
        target.write_text(target.read_text().replace('        id: expandedItem\n','        id: expandedItem\n'+probe,1))
        with (base/'session.log').open('w') as log:
            result=subprocess.run(['dbus-run-session','--',sys.executable,str(Path(__file__).resolve()),'--child',str(base)],env=env,stdout=log,stderr=log,timeout=90)
        output=(base/'session.log').read_text()
        if result.returncode:
            print(output)
            print((base/'plasma.log').read_text()[-16000:])
            raise SystemExit(result.returncode)
        print('\n'.join(line for line in output.splitlines() if line.startswith('PASS:')))

if __name__=='__main__': main()
