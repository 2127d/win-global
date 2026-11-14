#!/usr/bin/env python3
"""Build QRCA with a private copy of the shared Sequoia components."""
import argparse
from pathlib import Path
import subprocess
from installer_platform import application_version, build_jobs
ROOT = Path(__file__).resolve().parents[1]
VERSION = '26.08.2'

def installed_version():
    return application_version('qrca')

def build(folder, sdk=None, verify=False):
    if installed_version() != VERSION:
        raise RuntimeError('QRCA makeover requires Qrca ' + VERSION + '.')
    command = ['cmake', '--fresh', '-S', str(ROOT / 'apps/qrca' / ('qrca-' + VERSION)),
        '-B', str(folder), '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_INSTALL_PREFIX=/usr', '-DCMAKE_SKIP_RPATH=ON',
        '-DBUILD_TESTING=' + ('ON' if verify else 'OFF'),
        '-DSEQUOIA_QRCA_VERIFY=' + ('ON' if verify else 'OFF'),
        '-DSEQUOIA_COMPONENT_SOURCE=' + str(ROOT / 'desktop/components/library')]
    if sdk:
        command.append('-DCMAKE_PREFIX_PATH=' + str(sdk.resolve()))
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(folder), '--target', 'qrca',
        *(['qrcatest'] if verify else []), '--parallel', str(build_jobs())], check=True)
    return folder / 'bin/qrca'

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'apps/qrca/.build')
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    print(build(args.build_dir.resolve(), args.sdk, args.verify))
