#!/usr/bin/env python3
"""Build, test, and stage the native Sequoia Taskbar; never modify the desktop."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from installer_platform import build_jobs

ROOT = Path(__file__).resolve().parents[1]

def check_panel_backend():
    from PySide6.QtCore import QCoreApplication, QFile, QPluginLoader
    app = QCoreApplication.instance() or QCoreApplication([])
    plugin_dir = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
    loader = QPluginLoader(str(plugin_dir / 'plasma/applets/org.kde.panel.so'))
    if not loader.load():
        raise RuntimeError(loader.errorString())
    upstream = json.loads((ROOT / 'plasma/taskbar/upstream.json').read_text())
    for name, expected in upstream['files'].items():
        source = QFile(upstream['resource_prefix'] + name)
        if not source.open(QFile.ReadOnly) or hashlib.sha256(bytes(source.readAll())).hexdigest() != expected:
            raise RuntimeError(f'Installed panel differs from the supported Plasma {upstream["version"]} sources ({name}). Port the containment before building.')

def build(destination):
    if destination.exists():
        raise RuntimeError(f'Stage already exists: {destination}. Choose a new --stage directory.')
    check_panel_backend()
    build_dir = ROOT / '.staging/taskbar-build'
    subprocess.run(['cmake', '-S', str(ROOT / 'plasma/taskbar'), '-B', str(build_dir), '-G', 'Ninja',
        '-DCMAKE_INSTALL_PREFIX=/usr', '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_TESTING=ON'], check=True)
    subprocess.run(['cmake', '--build', str(build_dir), '--parallel', str(build_jobs())], check=True)
    subprocess.run(['ctest', '--test-dir', str(build_dir), '--output-on-failure'], check=True)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='sequoia-taskbar-stage-', dir=destination.parent) as temp:
        stage = Path(temp)
        subprocess.run(['cmake', '--install', str(build_dir)], env=dict(os.environ, DESTDIR=str(stage)), check=True)
        plugin_dir = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
        qml_dir = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_QML'], text=True).strip())
        fingerprint = plugin_dir / 'plasma/applets/org.kde.panel.so'
        manifest = {
            'format': 1,
            'backend': str(fingerprint),
            'backend_sha256': hashlib.sha256(fingerprint.read_bytes()).hexdigest(),
            'plugin_dir': str(plugin_dir),
            'qml_dir': str(qml_dir),
            'files': {str(p.relative_to(stage)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in sorted(stage.rglob('*')) if p.is_file()},
        }
        (stage / 'taskbar-build.json').write_text(json.dumps(manifest, indent=2) + '\n')
        stage.rename(destination)
    print(f'Staged taskbar: {destination}')
    print(f'Install: sudo python3 tools/install-taskbar.py --stage {destination}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, default=ROOT / '.staging/taskbar-install')
    build(parser.parse_args().stage.resolve())
