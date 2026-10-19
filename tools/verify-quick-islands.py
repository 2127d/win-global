#!/usr/bin/env python3
"""Check installation, launch arguments, update fallback, and exact restoration."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
installer = ROOT / 'tools/install-quick-islands.py'
with tempfile.TemporaryDirectory(prefix='sequoia-quick-check-') as temporary:
    prefix = Path(temporary) / 'data with spaces'
    desktop = prefix / 'applications/org.kde.plasma-systemmonitor.desktop'
    desktop.parent.mkdir(parents=True)
    original = b'[Desktop Entry]\nType=Application\nName=My System Monitor\nExec=plasma-systemmonitor --page-id history.page\nDBusActivatable=true\n'
    desktop.write_bytes(original)
    install = [sys.executable, str(installer), '--prefix', str(prefix), '--apps', 'systemmonitor']
    subprocess.run(install, check=True, stdout=subprocess.DEVNULL)
    target = prefix / 'sequoia-eleven/quick-islands'
    text = desktop.read_text()
    assert 'DBusActivatable=false' in text
    assert ' --page-id history.page' in text
    assert 'Exec="' in text
    probe = Path(temporary) / 'probe.py'
    probe.write_text('import json, os, sys; print(json.dumps({"args":sys.argv[1:], "qml":os.environ.get("QML_IMPORT_PATH"), "style":os.environ.get("QT_QUICK_CONTROLS_STYLE")}))')
    command = [str(target / 'sequoia-quick'), sys.executable, str(probe), 'argument with spaces', '--page-id', 'overview.page']
    env = dict(os.environ, QML_IMPORT_PATH='/existing/imports')
    result = subprocess.run(command, env=env, check=True, capture_output=True, text=True)
    data = json.loads(result.stdout)
    assert data['args'] == ['argument with spaces', '--page-id', 'overview.page']
    assert data['qml'] == str(target / 'qml') + os.pathsep + '/existing/imports'
    assert data['style'] == 'org.kde.desktop'
    subprocess.run(install, check=True, stdout=subprocess.DEVNULL)
    manifest_path = target / 'qml/manifest.json'
    manifest = json.loads(manifest_path.read_text())
    manifest['sources']['/nonexistent/kde-component'] = 'changed'
    manifest_path.write_text(json.dumps(manifest))
    result = subprocess.run(command, env=env, check=True, capture_output=True, text=True)
    assert json.loads(result.stdout)['qml'] == '/existing/imports'
    assert 'standard components' in result.stderr
    desktop.write_text(text + '# local edit\n')
    result = subprocess.run(install + ['--remove'], capture_output=True, text=True)
    assert result.returncode != 0 and 'edited after installation' in result.stderr
    desktop.write_text(text)
    subprocess.run(install + ['--remove'], check=True, stdout=subprocess.DEVNULL)
    assert desktop.read_bytes() == original
    assert not target.exists()
    # Both entry points use the shared navigation pages; Info Center is a
    # systemsettings symlink but must keep its own executable name/argv[0].
    for app, filename in [('systemsettings', 'systemsettings.desktop'),
                          ('kinfocenter', 'org.kde.kinfocenter.desktop')]:
        desktop = prefix / 'applications' / filename
        install = [sys.executable, str(installer), '--prefix', str(prefix), '--apps', app]
        subprocess.run(install, check=True, stdout=subprocess.DEVNULL)
        assert f'Exec="{target / "sequoia-quick"}" {app}\n' in desktop.read_text()
        subprocess.run(install, check=True, stdout=subprocess.DEVNULL)
        subprocess.run(install + ['--remove'], check=True, stdout=subprocess.DEVNULL)
        assert not desktop.exists()
    desktop = prefix / 'applications/org.kde.merkuro.calendar.desktop'
    install = [sys.executable, str(installer), '--prefix', str(prefix), '--apps', 'merkuro']
    subprocess.run(install, check=True, stdout=subprocess.DEVNULL)
    assert f'Exec="{target / "sequoia-quick"}" merkuro-calendar %u\n' in desktop.read_text()
    assert (target / 'qml/merkuro-qml.so').is_file()
    assert '/usr/bin/merkuro-calendar' in json.loads((target / 'qml/manifest.json').read_text())['sources']
    # Exercise the actual dynamic loader from a prefix containing spaces without
    # opening the calendar or accessing the user's accounts.
    probe = Path(temporary) / 'merkuro-calendar'
    probe.write_text('#!/usr/bin/python3\nfrom pathlib import Path\n'
                     'assert "merkuro-qml.so" in Path("/proc/self/maps").read_text()\n')
    probe.chmod(0o755)
    result = subprocess.run([str(target / 'sequoia-quick'), str(probe)],
                            env=env, capture_output=True, text=True, check=True)
    assert 'cannot be preloaded' not in result.stderr, result.stderr
    subprocess.run(install + ['--remove'], check=True, stdout=subprocess.DEVNULL)
    assert not desktop.exists()
print('PASS: installation/refresh, paths with spaces, argument forwarding, import precedence, stale-version fallback, edit protection, exact restoration')
