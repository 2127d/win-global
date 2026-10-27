#!/usr/bin/env python3
"""Verify a portable installer after extraction into a path containing spaces."""
import argparse
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--full', action='store_true', help='Also build native components and test install/refresh/removal')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='sequoia extracted installer ') as temporary:
        base = Path(temporary)
        with tarfile.open(args.archive.resolve()) as archive:
            archive.extractall(base, filter='data')
        project = base / 'Sequoia-Eleven'
        for relative in ('themes/application-style/SequoiaEleven/CMakeLists.txt',
                         'themes/plasma/Splash.qml', 'widgets/plasmoids/org.sequoia.startmenu/metadata.json',
                         'desktop/taskbar/CMakeLists.txt', 'tools/project_paths.py', 'docs/DEPLOYMENT.md',
                         'install.sh', 'install-gui.py', 'uninstall-gui.py', 'tools/gtk_icons.py'):
            assert (project / relative).is_file(), relative
        for name in ('settings', 'gwenview', 'kcalc'):
            assert list((project / 'apps' / name).glob('*/CMakeLists.txt')), name
        for path in project.rglob('*'):
            if path.is_symlink():
                assert path.exists() and path.resolve().is_relative_to(project), path
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
                   QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion',
                   SEQUOIA_FULL_INSTALL_TEST='1' if args.full else '0')
        home = base / 'guest home'
        home.mkdir()
        env['HOME'] = str(home)
        for kind in ('CONFIG', 'DATA', 'STATE', 'CACHE', 'RUNTIME'):
            directory = home / kind.lower()
            directory.mkdir(mode=0o700)
            env['XDG_' + kind + ('_DIR' if kind == 'RUNTIME' else '_HOME')] = str(directory)
        env['DBUS_SESSION_BUS_ADDRESS'] = 'unix:path=' + str(base / 'no-live-session')
        for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'LD_PRELOAD', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH', 'PYTHONPATH'):
            env.pop(name, None)
        for script in ('verify-gtk-icons.py', 'verify-theme-installer.py', 'verify-installer-gui.py',
                       'verify-uninstaller-gui.py', 'verify-desktop-layout.py'):
            subprocess.run(['python3', str(project / 'tools' / script)], cwd=base,
                           env=env, check=True, timeout=900)
        print('PASS: extracted archive, all app sources, internal symlinks, GUI, installation and removal; paths with spaces')


if __name__ == '__main__':
    main()
