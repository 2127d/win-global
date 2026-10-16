#!/usr/bin/env python3
"""Install a verified taskbar stage, or remove owned files with backup restoration."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from theme_files import Receipt, payload_files

STATE = 'share/sequoia-eleven/taskbar/installation'

def allowed(name):
    return (name.startswith(('share/plasma/plasmoids/org.sequoia.taskbar/',
                             'share/plasma/layout-templates/org.sequoia.taskbar.panel/'))
        or name == 'share/applications/kcm_sequoia_taskbar.desktop'
        or (name.startswith(('lib/', 'lib64/')) and (
            name.endswith('/qt6/plugins/plasma/applets/org.sequoia.taskbar.so')
            or name.endswith('/qt6/plugins/plasma/kcms/systemsettings/kcm_sequoia_taskbar.so')
            or '/qt6/qml/org/sequoia/taskbar/' in name)))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--stage', type=Path)
    mode.add_argument('--remove', action='store_true')
    parser.add_argument('--prefix', type=Path, default=Path('/usr'))
    args = parser.parse_args()
    receipt = Receipt(args.prefix, STATE, allowed)
    if args.remove:
        receipt.uninstall()
        print('Removed the taskbar files; saved appearance preferences were kept.')
        return
    stage = args.stage.resolve()
    manifest = json.loads((stage / 'taskbar-build.json').read_text())
    if manifest.get('format') != 1:
        raise RuntimeError('Unsupported taskbar stage format')
    plugin_dir = Path(subprocess.check_output(['qtpaths6', '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
    backend = plugin_dir / 'plasma/applets/org.kde.panel.so'
    if hashlib.sha256(backend.read_bytes()).hexdigest() != manifest['backend_sha256']:
        raise RuntimeError('Plasma changed since this stage was built; build and verify a new stage.')
    actual = {str(p.relative_to(stage)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in stage.rglob('*') if p.is_file() and p.name != 'taskbar-build.json'}
    if actual != manifest['files']:
        raise RuntimeError('Taskbar stage was modified after building.')
    if any(not allowed(name) for name in payload_files(stage / 'usr')):
        raise RuntimeError('Stage contains unexpected installation paths.')
    receipt.install(stage / 'usr')
    print('Installed Sequoia Taskbar. Log out and back in, then use Add Panel → Sequoia Taskbar.')
    print('Configure it in System Settings → Appearance → Taskbar. Existing panels were preserved.')

if __name__ == '__main__':
    main()
