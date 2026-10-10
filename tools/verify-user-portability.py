#!/usr/bin/env python3
"""Test per-user installation in disposable homes; desktop activation is stubbed."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import traceback

ROOT = Path(__file__).resolve().parents[1]


def fingerprint(path):
    if not path.exists():
        return None
    if path.is_file():
        return hashlib.sha256(path.read_bytes()).hexdigest()
    return {str(p.relative_to(path)): fingerprint(p) for p in path.iterdir()}


def run(env, cwd, *command):
    result = subprocess.run([str(arg) for arg in command], cwd=cwd, env=env,
                            capture_output=True, text=True, timeout=180)
    if result.returncode:
        raise RuntimeError(f'{command}:\n{result.stdout}\n{result.stderr}')
    return result.stdout


def verify_account(base, layout, optional=False):
    home = base / 'guest home'
    home.mkdir(parents=True)
    config = base / 'external settings' if layout == 'external' else home / '.config'
    data = base / 'external data' if layout == 'external' else home / '.local/share'
    config.mkdir(parents=True)
    data.mkdir(parents=True)
    working = base / 'unrelated working directory'
    working.mkdir()
    commands = base / 'commands'
    commands.mkdir()
    log = base / 'activation.jsonl'
    # Real file writes, font installation, KConfig writes and snapshots. Only
    # commands which activate the desktop or contact session services are fakes.
    for command in ('plasma-apply-colorscheme', 'plasma-apply-desktoptheme',
                    'plasma-apply-wallpaperimage', 'qdbus-qt6', 'dbus-send'):
        executable = commands / command
        executable.write_text('#!/usr/bin/python3\nimport json, os, sys\n'
                              'with open(os.environ["ACTIVATION_LOG"], "a") as log:\n'
                              '    log.write(json.dumps(sys.argv) + "\\n")\n')
        executable.chmod(0o755)
    env = os.environ.copy()
    for key in ('XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'DBUS_SESSION_BUS_ADDRESS',
                'DISPLAY', 'WAYLAND_DISPLAY', 'QT_PLUGIN_PATH', 'LD_PRELOAD'):
        env.pop(key, None)
    runtime = base / 'runtime'
    runtime.mkdir(mode=0o700)
    env.update(HOME=str(home), XDG_CACHE_HOME=str(base / 'cache'),
               XDG_STATE_HOME=str(base / 'state'), XDG_RUNTIME_DIR=str(runtime),
               XDG_CONFIG_DIRS='/etc/xdg', XDG_DATA_DIRS='/usr/local/share:/usr/share',
               QT_QPA_PLATFORM='offscreen',
               DBUS_SESSION_BUS_ADDRESS='unix:path=' + str(base / 'no-session-bus'),
               PATH=str(commands) + os.pathsep + env['PATH'], ACTIVATION_LOG=str(log))
    if layout == 'external':
        env.update(XDG_CONFIG_HOME=str(config), XDG_DATA_HOME=str(data))
    elif layout == 'empty':
        env.update(XDG_CONFIG_HOME='', XDG_DATA_HOME='')
    (config / 'kdeglobals').write_text('[Unrelated]\nKeep=guest value\n')
    (config / 'kwinrc').write_text('[Unrelated]\nKeep=guest window setting\n')
    (config / 'plasma-org.kde.plasma.desktop-appletsrc').write_text('[Containments][1]\nplugin=org.kde.panel\n')
    original = {str(config / name): fingerprint(config / name) for name in
                ('kdeglobals', 'kwinrc', 'plasma-org.kde.plasma.desktop-appletsrc')}
    if optional:
        verify_optional(env, working, config, data, commands, layout)
        return
    # Activation is separate from installation and must not rebuild or use sudo.
    run(env, working, 'bash', ROOT / 'tools/install-theme-assets.sh')
    run(env, working, 'python3', ROOT / 'tools/install-fonts.py')
    run(env, working, 'python3', ROOT / 'tools/install-lockscreen.py')
    run(env, working, 'python3', ROOT / 'tools/apply-desktop.py', '--mode', 'dark')
    assert (data / 'icons/SequoiaTahoe-dark/index.theme').is_file()
    assert (data / 'plasma/shells/org.kde.plasma.desktop/.sequoia-eleven-lockscreen').is_file()
    assert (data / 'fonts/sequoia-eleven/AdwaitaSans/sources.json').is_file()
    globals_text = (config / 'kdeglobals').read_text()
    assert 'Keep=guest value' in globals_text
    assert run(env, working, 'kreadconfig6', '--file', config / 'kdeglobals',
               '--group', 'KDE', '--key', 'widgetStyle').strip() == 'SequoiaEleven'
    calls = [json.loads(line) for line in log.read_text().splitlines()]
    assert any(call[1:] == [str(data / 'wallpapers/SequoiaEleven')] for call in calls)
    backup, = (data / 'sequoia-eleven/backups').iterdir()
    manifest = json.loads((backup / 'manifest.json').read_text())
    for entry in manifest:
        path = Path(entry['path'])
        assert path.is_relative_to(config) or path.is_relative_to(data), path
        assert entry['exists'] == (str(path) in original), path
    run(env, working, 'python3', ROOT / 'tools/restore-desktop.py', backup)
    for entry in manifest:
        assert fingerprint(Path(entry['path'])) == original.get(entry['path']), entry['path']
    run(env, working, 'python3', ROOT / 'tools/apply-fonts.py', '--no-notify')
    font_backup, = (data / 'sequoia-eleven/backups').glob('fonts-*')
    assert (config / 'fontconfig/conf.d/99-sequoia-ui-fonts.conf').is_file()
    run(env, working, 'python3', ROOT / 'tools/restore-desktop.py', font_backup)
    assert fingerprint(config / 'kdeglobals') == original[str(config / 'kdeglobals')]
    assert not (config / 'fontconfig/conf.d/99-sequoia-ui-fonts.conf').exists()
    assert not list(working.iterdir()), 'Installer wrote into the working directory'
    if layout == 'external':
        assert not (home / '.config').exists() and not (home / '.local/share').exists()
    print(f'PASS: {layout} HOME/XDG layout, desktop and font installation/restoration, paths with spaces', flush=True)


def verify_optional(env, working, config, data, commands, layout):
    # These responses emulate an existing dock; panel JavaScript itself is not
    # executed. They exercise the real installers' filesystem/account handling.
    qdbus = commands / 'qdbus-qt6'
    qdbus.write_text('''#!/usr/bin/python3
import sys
code = sys.argv[-1]
if 'Dock and menu bar refinements applied.' in code:
    print('Dock and menu bar refinements applied.')
elif 'JSON.stringify({panel:' in code:
    print('{"panel": 1, "stack": 2}')
elif 'Downloads fan installed.' in code:
    print('Downloads fan installed.')
else:
    sys.exit('Unexpected Plasma script')
''')
    for name in ('systemctl', 'kbuildsycoca6'):
        target = commands / name
        target.write_text('#!/bin/sh\nexit 0\n')
        target.chmod(0o755)
    (config / 'plasmashellrc').write_text('[Unrelated]\nKeep=guest shell setting\n')
    (config / 'user-dirs.dirs').write_text('XDG_DOWNLOAD_DIR="$HOME/Incoming files"\n')
    panel_bytes = (config / 'plasma-org.kde.plasma.desktop-appletsrc').read_bytes()
    run(env, working, 'python3', ROOT / 'tools/apply-mac-panels.py')
    run(env, working, 'python3', ROOT / 'tools/install-downloads-stack.py')
    for pattern in ('before-mac-panels-*', 'before-downloads-fan-*'):
        backup, = (data / 'sequoia-eleven/backups').glob(pattern)
        assert (backup / 'plasma-org.kde.plasma.desktop-appletsrc').read_bytes() == panel_bytes
    service = config / 'systemd/user/plasma-plasmashell.service.d/60-sequoia-downloads-style.conf'
    assert str(data / 'sequoia-eleven/downloads-style') in service.read_text()
    run(env, working, 'python3', ROOT / 'tools/install-widgets.py', '--check')
    # A fixture package exercises the optional third-party Game Drawer patch.
    drawer = data / 'plasma/plasmoids/com.atopion.gamedrawer/contents/ui/main.qml'
    drawer.parent.mkdir(parents=True)
    previous = 'Item {\n    fullRepresentation: PlasmaComponents3.Button {\n    }\n}\n'
    drawer.write_text(previous)
    run(env, working, 'python3', ROOT / 'tools/install-gamedrawer.py')
    assert drawer.with_suffix('.qml.before-compact-highlight').read_text() == previous
    assert 'topInset: 6' in drawer.read_text()
    # Installation/build coverage already exists for these two extensions.
    # Probe the changed default path handling through their removal branches.
    quick = data / 'sequoia-eleven/quick-islands'
    quick.mkdir(parents=True)
    (quick / 'installation.json').write_text('{"launchers": {}}\n')
    run(env, working, 'python3', ROOT / 'tools/install-quick-islands.py', '--remove')
    assert not quick.exists()
    overlay = data / 'sequoia-eleven/control-center'
    overlay.mkdir(parents=True)
    (overlay / '.sequoia-control-center').write_text('1\n')
    dropin = service.with_name('50-sequoia-control-center.conf')
    dropin.write_text('# Sequoia Eleven Control Center\n[Service]\n')
    run(env, working, 'python3', ROOT / 'tools/install-control-center.py', '--remove')
    assert not overlay.exists() and not dropin.exists()
    assert not list(working.iterdir())
    print(f'PASS: {layout} optional panel/download paths and backups, KPackage discovery, Game Drawer, extension removal paths', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--optional', action='store_true', help='Test optional tools instead of the desktop/font workflow')
    args = parser.parse_args()
    errors = []
    with tempfile.TemporaryDirectory(prefix='sequoia-user-portability-') as temporary:
        for layout in ('default', 'external', 'empty'):
            try:
                verify_account(Path(temporary) / layout, layout, args.optional)
            except Exception as error:
                errors.append(f'{layout}: {traceback.format_exc()}')
    if errors:
        raise RuntimeError('\n'.join(errors))
    print('Desktop activation calls were recorded, not executed. No live account was modified.')


if __name__ == '__main__':
    main()
