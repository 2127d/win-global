#!/usr/bin/env python3
"""Private compositor for actual Gwenview popup/input lifetime tests."""
import os
from pathlib import Path
import subprocess
import sys
import time
import shlex
import tempfile

env = dict(os.environ)
runtime = Path(env['XDG_RUNTIME_DIR'])
assert runtime.parent.parent == Path(tempfile.gettempdir()) and runtime.parent.name.startswith('gwenview-test-')
(Path(env['XDG_CONFIG_HOME']) / 'kwinrc').write_text('[Plugins]\nblurEnabled=' + ('false' if env.get('SEQUOIA_GWENVIEW_TEST_BLUR') == '0' else 'true') + '\n')
compositor_env = dict(env, QT_QPA_PLATFORM='offscreen', KWIN_SCREENSHOT_NO_PERMISSION_CHECKS='1')
for key in ('LD_PRELOAD', 'WAYLAND_DISPLAY', 'DISPLAY', 'QT_PLUGIN_PATH'):
    compositor_env.pop(key, None)
with (runtime.parent / 'compositor.log').open('w') as log:
    compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', '1600', '--height', '1200',
        '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities', '--socket', 'gwenview-menu-test'],
        env=compositor_env, stdout=log, stderr=log)
    try:
        for attempt in range(100):
            if (runtime / 'gwenview-menu-test').exists(): break
            if compositor.poll() is not None: raise RuntimeError('Private compositor exited')
            time.sleep(.05)
        else: raise RuntimeError('Private compositor did not start')
        env.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='gwenview-menu-test', SEQUOIA_GWENVIEW_NATIVE_INPUT='1')
        shell = None
        registrar = None
        shell_log = None
        if env.get('SEQUOIA_GWENVIEW_GLOBAL_MENU'):
            # Qt enables its native menu exporter only while the registrar
            # service exists. On Wayland KWin carries the menu address, so no
            # X11 RegisterWindow methods are needed in this private session.
            registrar = subprocess.Popen([sys.executable, '-c',
                'from PySide6.QtCore import QCoreApplication; from PySide6.QtDBus import QDBusConnection; '
                'app=QCoreApplication([]); assert QDBusConnection.sessionBus().registerService("com.canonical.AppMenu.Registrar"); app.exec()'], env=compositor_env)
            time.sleep(.2)
            root = Path(__file__).resolve().parents[1]
            probe = root / '.staging/gwenview-global-menu-probe.so'
            flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets'], text=True))
            subprocess.run(['c++', '-shared', '-fPIC', str(root / 'tools/test-support/gwenview-global-menu-probe.cpp'), '-o', str(probe), *flags], check=True)
            shell_log = (runtime.parent / 'global-menu.log').open('w+')
            shell_env = dict(env, LD_PRELOAD=str(probe), SEQUOIA_GLOBAL_MENU_PROBE='1', KCOREADDONS_NO_XDG_DESKTOP_PORTAL='1', QT_NO_XDG_DESKTOP_PORTAL='1')
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=shell_env, stdout=shell_log, stderr=shell_log)
            for attempt in range(100):
                query = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript', 'print(desktops().length);'], capture_output=True, text=True)
                if query.returncode == 0 and query.stdout.strip() == '1': break
                if shell.poll() is not None: raise RuntimeError('Private shell exited')
                time.sleep(.1)
            subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell', 'org.kde.PlasmaShell.evaluateScript',
                'panels().forEach(p=>p.remove()); var p=new Panel; p.location="top"; p.height=34; p.addWidget("org.kde.plasma.appmenu");'], check=True)
        try:
            result = subprocess.run(sys.argv[1:], env=env)
            if shell:
                try: shell.wait(timeout=3)
                except subprocess.TimeoutExpired: shell.terminate(); shell.wait(timeout=3)
                shell_log.seek(0); log_text = shell_log.read(); print(log_text, flush=True)
                if result.returncode == 0 and (shell.returncode != 0 or 'PASS:' not in log_text):
                    raise RuntimeError('Private global-menu check failed')
        finally:
            if shell and shell.poll() is None: shell.terminate(); shell.wait(timeout=5)
            if shell_log:
                shell_log.seek(0); print(shell_log.read(), flush=True); shell_log.close()
            if registrar: registrar.terminate(); registrar.wait(timeout=3)
    finally:
        compositor.terminate()
        try: compositor.wait(timeout=5)
        except subprocess.TimeoutExpired:
            compositor.kill(); compositor.wait()
sys.exit(result.returncode)
