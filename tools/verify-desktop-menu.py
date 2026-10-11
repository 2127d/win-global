#!/usr/bin/env python3
"""Exercise the desktop menu in an isolated Plasma applet host."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = 'org.sequoia.desktopmenu'


def native_child(library):
    stage = Path(os.environ['XDG_RUNTIME_DIR']).parent
    compositor = shell = None
    with (stage / 'kwin.log').open('w') as log:
        try:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1280', '--height', '720',
                '--no-lockscreen', '--no-global-shortcuts', '--socket', 'desktop-menu-test'], stdout=log, stderr=log)
            for _ in range(100):
                if (stage / 'runtime/desktop-menu-test').exists():
                    break
                time.sleep(.1)
            env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='desktop-menu-test',
                       LD_PRELOAD=library, SEQUOIA_DESKTOP_MENU_NATIVE='1')
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=env)
            for _ in range(100):
                query = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                    'org.kde.PlasmaShell.evaluateScript', 'print(desktops().length);'], capture_output=True, text=True)
                if query.returncode == 0 and query.stdout.strip() == '1':
                    break
                time.sleep(.1)
            subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                'org.kde.PlasmaShell.evaluateScript',
                'panels().forEach(p=>p.remove()); var p=new Panel; p.location="top"; p.height=34;'
                'var t=p.addWidget("org.kde.windowtitle.Fork"); t.currentConfigGroup=["General"];'
                't.writeConfig("customText",true);t.writeConfig("textDefault","");t.writeConfig("showIcon",false);t.reloadConfig();'
                'p.addWidget("org.sequoia.desktopmenu"); p.addWidget("org.kde.plasma.appmenu");'
                'p.addWidget("org.kde.plasma.panelspacer");'], check=True)
            return shell.wait(timeout=35)
        finally:
            for process in (shell, compositor):
                if process and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()


def main():
    library = ROOT / '.staging/verify-desktop-menu.so'
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6Test'], text=True))
    subprocess.run(['c++', '-std=c++17', '-fPIC', '-shared',
        str(ROOT / 'tools/verify-desktop-menu.cpp'), '-o', str(library), *flags], check=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-desktop-menu-', ignore_cleanup_errors=True) as directory:
        stage = Path(directory)
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                   QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion', QT_NO_XDG_DESKTOP_PORTAL='1',
                   QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
                   QT_FORCE_STDERR_LOGGING='1',
                   SEQUOIA_DESKTOP_MENU_CAPTURE=str(ROOT / 'preview/desktop-menu'))
        for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH', 'QT_PLUGIN_PATH'):
            env.pop(key, None)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = stage / kind.lower()
            path.mkdir(mode=0o700)
            env[f'XDG_{kind}_' + ('DIR' if kind == 'RUNTIME' else 'HOME')] = str(path)
        shutil.copytree(ROOT / 'plasma/plasmoids' / PLUGIN, stage / 'data/plasma/plasmoids' / PLUGIN)
        shutil.copytree(ROOT / 'plasma/plasmoids/org.kde.windowtitle.Fork',
                        stage / 'data/plasma/plasmoids/org.kde.windowtitle.Fork')
        shutil.copytree(ROOT / 'plasma/desktoptheme/sequoia-eleven-light',
                        stage / 'data/plasma/desktoptheme/sequoia-eleven-light')
        (stage / 'config/plasmarc').write_text('[Theme]\nname=sequoia-eleven-light\n')
        shutil.copy2(ROOT / 'color-schemes/SequoiaElevenLight.colors', stage / 'config/kdeglobals')
        logfile = ROOT / '.staging/desktop-menu-test.log'
        with logfile.open('w') as log:
            command = ([sys.executable, str(Path(__file__).resolve()), '--native-child', str(library)]
                       if '--native' in sys.argv else ['env', f'LD_PRELOAD={library}', 'plasmawindowed', PLUGIN])
            result = subprocess.run(['dbus-run-session', '--', *command],
                env=env, stdout=log, stderr=log, timeout=55)
        output = logfile.read_text()
        # Stock desktop settings can emit teardown warnings when the private
        # shell exits. Fail on errors in this package or our QML expressions.
        errors = [line for line in output.splitlines()
                  if (PLUGIN in line or 'org.kde.windowtitle.Fork' in line or '<Unknown File>' in line) and any(word in line for word in
                      ('ReferenceError:', 'TypeError:', 'Cannot assign', 'Unable to assign',
                       'is not a type', 'unavailable', 'error when loading'))]
        if result.returncode or errors or 'PASS:' not in output:
            raise RuntimeError(output)
        print('\n'.join(line for line in output.splitlines() if line.startswith('PASS:')))


if __name__ == '__main__':
    if '--native-child' in sys.argv:
        raise SystemExit(native_child(sys.argv[-1]))
    main()
