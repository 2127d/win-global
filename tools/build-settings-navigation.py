#!/usr/bin/env python3
"""Build the navigation fork against the installed KDE development libraries."""
import argparse
import os
from pathlib import Path
import subprocess
from installer_platform import build_jobs

ROOT = Path(__file__).resolve().parents[1]
VERSION = '6.7.5'


def installed_version():
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
    env.pop('LD_PRELOAD', None)
    result = subprocess.check_output(['/usr/bin/systemsettings', '--version'], env=env, text=True, timeout=15)
    return result.strip().split()[-1]


def build(build_dir, sdk=None):
    jobs = build_jobs()
    print(f'Building System Settings with {jobs} parallel jobs.', flush=True)
    if installed_version() != VERSION:
        raise RuntimeError(f'This source matches KDE System Settings {VERSION}; update the fork before building for another version.')
    # Rediscover dependencies: an earlier --sdk may point into expired /tmp data.
    command = ['cmake', '--fresh', '-S', str(ROOT / f'apps/settings/systemsettings-v{VERSION}'), '-B', str(build_dir),
               '-G', 'Ninja', '-DBUILD_TESTING=OFF', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_INSTALL_PREFIX=/usr']
    if sdk:
        command.append('-DCMAKE_PREFIX_PATH=' + str(sdk))
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(build_dir), '--target', 'systemsettings', '--parallel', str(jobs)], check=True)
    return build_dir / 'bin/systemsettings'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/settings-build')
    parser.add_argument('--sdk', type=Path, help='Optional prefix containing development headers and CMake packages.')
    args = parser.parse_args()
    print(build(args.build_dir.resolve(), args.sdk))
