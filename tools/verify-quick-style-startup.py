#!/usr/bin/env python3
"""Exercise automatic Qt Quick activation through the compiled native style."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sequoia-quick-startup-') as temporary:
    base = Path(temporary)
    data = base / 'data with spaces'
    overlay = data / 'sequoia-eleven/quick-islands/qml'
    overlay.mkdir(parents=True)
    # Share the real generated components but control the version manifest.
    source = ROOT / '.staging/quick-islands'
    for path in source.iterdir():
        if path.name != 'manifest.json':
            (overlay / path.name).symlink_to(path, target_is_directory=path.is_dir())
    fingerprinted = base / 'component.qml'
    fingerprinted.write_text('current KDE component')
    manifest = overlay / 'manifest.json'
    manifest.write_text(json.dumps({'sources': {
        str(fingerprinted): hashlib.sha256(fingerprinted.read_bytes()).hexdigest()}}))
    probe = base / 'probe'
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Qml', 'Qt6Quick', 'Qt6QuickControls2'], text=True))
    subprocess.run(['c++', '-std=c++17', '-fPIC', str(ROOT / 'tools/verify-quick-style-startup.cpp'),
                    '-o', str(probe), *flags], check=True)
    env = dict(os.environ, XDG_DATA_HOME=str(data), XDG_CONFIG_HOME=str(base / 'config'),
               XDG_CACHE_HOME=str(base / 'cache'), QT_QPA_PLATFORM='offscreen',
               QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='SequoiaEleven',
               QT_PLUGIN_PATH=str(ROOT / '.staging/desktop/usr/lib64/qt6/plugins'),
               QT_QUICK_CONTROLS_STYLE='org.kde.desktop',
               QML_IMPORT_PATH='/existing/imports', QML2_IMPORT_PATH='/legacy/imports')
    env.pop('LD_PRELOAD', None)

    def check(name, styled, overrides=None):
        executable = base / name
        shutil.copy2(probe, executable)
        result = subprocess.run([str(executable)], env=env | (overrides or {}),
                                capture_output=True, text=True, timeout=25)
        assert result.returncode == 0, result.stderr + result.stdout
        result = json.loads(result.stdout)
        assert (result['background'] == 'sequoiaCanvas') == styled, result
        for key, original in [('qml', '/existing/imports'), ('qml2', '/legacy/imports')]:
            assert result[key] == (str(overlay) + os.pathsep if styled else '') + original, result
        assert result['style'] == 'org.kde.desktop', result
        if styled:
            assert result['selectedStyle'] == 'org.kde.desktop', result

    for name in ('systemsettings', 'kinfocenter', 'plasma-systemmonitor',
                 'plasma-discover', 'kcmshell6', 'merkuro-calendar'):
        check(name, True)
    # Empty style tests native auto-selection rather than wrapper configuration.
    check('systemsettings', True, {'QT_QUICK_CONTROLS_STYLE': ''})
    for name in ('plasmashell', 'plasmawindowed', 'plasma-login-greeter', 'unrelated-app'):
        check(name, False)
    fingerprinted.write_text('upgraded KDE component')
    check('systemsettings', False)
    manifest.write_text('{broken json')
    check('systemsettings', False)
    manifest.write_text('{"sources": {}}')
    check('systemsettings', False)
    manifest.unlink()
    check('systemsettings', False)
print('PASS: direct native-style activation, real QML loading, path preservation/deduplication, '
      'Plasma isolation, and missing/invalid/stale overlay fallback')
