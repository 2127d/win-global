#!/usr/bin/env python3
"""Test the real picker in private Wayland sessions; never modify the live desktop."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import shell_overrides

ROOT = Path(__file__).resolve().parents[1]


def stop(process):
    if process and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def child(base, width, height, offset):
    compositor = shell = None
    with (base / 'plasma.log').open('w') as log:
        try:
            compositor = subprocess.Popen(['kwin_wayland', '--virtual', '--width', str(width), '--height', str(height),
                '--output-count', '2' if offset else '1',
                '--no-lockscreen', '--no-global-shortcuts', '--socket', 'picker-test'], stdout=log, stderr=log)
            for _ in range(100):
                if (base / 'runtime/picker-test').exists():
                    break
                time.sleep(.1)
            env = dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='picker-test',
                       LD_PRELOAD=str(ROOT / '.staging/verify-widget-picker.so'), SEQUOIA_PICKER_READY=str(base / 'ready'))
            if offset:
                env['SEQUOIA_PICKER_SECOND_OUTPUT'] = '1'
                subprocess.run(['kscreen-doctor', '-o', f'output.Virtual-2.position.{offset},120'],
                               env=dict(os.environ, QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY='picker-test', QT_LOGGING_RULES='*.debug=true'),
                               check=True, stdout=log, stderr=log)
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=env, stdout=log, stderr=log)
            for _ in range(100):
                result = subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                    'org.kde.PlasmaShell.evaluateScript', 'print(desktops().length);'], capture_output=True, text=True)
                if result.returncode == 0 and result.stdout.strip() == ('2' if offset else '1'):
                    break
                time.sleep(.1)
            subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                'org.kde.PlasmaShell.evaluateScript', 'panels().forEach(p=>p.remove()); var p = new Panel; p.location = "bottom"; p.height = 40;'], check=True, capture_output=True)
            (base / 'ready').touch()
            if not offset:
                subprocess.run(['qdbus-qt6', 'org.kde.plasmashell', '/PlasmaShell',
                    'org.kde.PlasmaShell.toggleWidgetExplorer'], check=True, capture_output=True)
            result = shell.wait(timeout=30)
            if result:
                return result
            # Start a new shell against the same private config to verify that
            # Plasma persisted the resized and dragged footprints and positions.
            env['SEQUOIA_GRID_RESTORE'] = '1'
            shell = subprocess.Popen(['plasmashell', '--no-respawn'], env=env, stdout=log, stderr=log)
            return shell.wait(timeout=30)
        finally:
            stop(shell)
            stop(compositor)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--child', type=Path)
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    parser.add_argument('--scale', type=float, default=1)
    parser.add_argument('--offset', type=int, default=0)
    parser.add_argument('--mode', choices=('light', 'dark'), default='dark')
    parser.add_argument('--material', choices=('solid', 'transparent'), default='solid')
    args = parser.parse_args()
    if args.child:
        raise SystemExit(child(args.child, args.width, args.height, args.offset))
    library = ROOT / '.staging/verify-widget-picker.so'
    library.parent.mkdir(exist_ok=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Widgets', 'Qt6Test', 'Qt6DBus'], text=True))
    ei = subprocess.run(['pkg-config', '--cflags', '--libs', 'libei-1.0'], text=True, capture_output=True)
    if ei.returncode == 0:
        flags += shlex.split(ei.stdout)
    elif (ROOT / '.staging/test-sdk/libei.h').is_file():
        flags += ['-I' + str(ROOT / '.staging/test-sdk'), '-l:libei.so.1']
    else:
        raise RuntimeError('Install the libei development package to run compositor input tests.')
    source = ROOT / 'tools/verify-widget-picker.cpp'
    support = ROOT / 'tools/test-support'
    if not library.exists() or max(p.stat().st_mtime for p in [source, *support.iterdir()]) > library.stat().st_mtime:
        subprocess.run(['c++', '-std=c++17', '-fPIC', '-shared', str(source), '-o', str(library), *flags], check=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-picker-', ignore_cleanup_errors=True) as directory:
        base = Path(directory)
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                   QT_QPA_PLATFORMTHEME='kde', QT_STYLE_OVERRIDE='Fusion', KWIN_COMPOSE='Q',
                   QT_FORCE_STDERR_LOGGING='1', QT_SCALE_FACTOR=str(args.scale),
                   SEQUOIA_PICKER_MATERIAL=args.material,
                   SEQUOIA_CLOCK_CAPTURE=str(ROOT / f'preview/analog-clock-{args.mode}'),
                   SEQUOIA_PICKER_CAPTURE=str(ROOT / f'preview/widget-picker-{args.width}x{args.height}-{args.mode}-{args.material}-scale{args.scale:g}.png'))
        for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QML_IMPORT_PATH', 'QT_PLUGIN_PATH'):
            env.pop(key, None)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            path = base / kind.lower()
            path.mkdir(mode=0o700)
            env[f'XDG_{kind}_' + ('DIR' if kind == 'RUNTIME' else 'HOME')] = str(path)
        # Include the user's installed widget catalog read-only, after the private shell.
        env['XDG_DATA_DIRS'] = str(Path.home() / '.local/share') + ':/usr/local/share:/usr/share'
        env['XDG_CONFIG_DIRS'] = '/etc/xdg'
        env['AKONADI_INSTANCE'] = base.name
        shell_overrides.stage(base / 'data', base / 'unused', {'plasma-style'}, {})
        shutil.copytree(ROOT / 'widgets/plasmoids/org.sequoia.gridtest',
                        base / 'data/plasma/plasmoids/org.sequoia.gridtest')
        shutil.copytree(ROOT / 'widgets/plasmoids/org.sequoia.analogclock',
                        base / 'data/plasma/plasmoids/org.sequoia.analogclock')
        theme = f'sequoia-eleven-{args.mode}'
        shutil.copytree(ROOT / 'themes/plasma/desktoptheme' / theme, base / 'data/plasma/desktoptheme' / theme)
        (base / 'config/plasmarc').write_text(f'[Theme]\nname={theme}\n')
        (base / 'config/plasma-welcomerc').write_text('[General]\nLastSeenVersion=999.0\n')
        shutil.copy2(ROOT / f'themes/color-schemes/SequoiaEleven{args.mode.title()}{args.material.title()}.colors', base / 'config/kdeglobals')
        with (base / 'session.log').open('w') as log:
            result = subprocess.run(['dbus-run-session', '--', sys.executable, str(Path(__file__).resolve()),
                '--child', str(base), '--width', str(args.width), '--height', str(args.height), '--offset', str(args.offset)],
                env=env, stdout=log, stderr=log, timeout=55)
        if not (base / 'plasma.log').exists():
            raise RuntimeError((base / 'session.log').read_text())
        output = (base / 'plasma.log').read_text()
        errors = [line for line in output.splitlines() if any(name in line for name in
            ('Desktop.qml', 'WidgetExplorer.qml', 'AppletDelegate.qml', 'PickerMetrics.qml',
             'GridDesktop.qml', 'GridSession.qml', 'GridTarget.qml', 'GridItem.qml', 'GridGeometry.qml',
             'PlacementWizard.qml', 'PlacementDialog.qml', 'GridTestFace.qml', 'org.sequoia.gridtest',
             'org.sequoia.analogclock', 'ClockFace.qml', 'AnalogClockPreview.qml'))
            and any(word in line for word in ('Error:', 'Cannot assign', 'Unable to assign', 'unavailable', 'Binding loop'))]
        if result.returncode or errors or 'PASS:' not in output:
            print((base / 'session.log').read_text())
            raise RuntimeError(output)
        print('\n'.join(line for line in output.splitlines() if line.startswith(('PASS:', 'GEOMETRY:', 'MATERIAL:'))))


if __name__ == '__main__':
    main()
