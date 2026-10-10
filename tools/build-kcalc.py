#!/usr/bin/env python3
"""Build the KCalc presentation override with the installed KDE libraries."""
import argparse
from pathlib import Path
import subprocess
from installer_platform import application_version, build_jobs

ROOT = Path(__file__).resolve().parents[1]
VERSION = '26.08.1'


def installed_version():
    return application_version('kcalc')


def build(folder, sdk=None):
    if installed_version() != VERSION:
        raise RuntimeError('Update the KCalc source fork to match the installed version first.')
    jobs = build_jobs()
    print(f'Building KCalc with {jobs} parallel jobs.', flush=True)
    command = ['cmake', '-S', str(ROOT / f'apps/kcalc/kcalc-{VERSION}'), '-B', str(folder),
               '-G', 'Ninja', '-DBUILD_TESTING=OFF', '-DCMAKE_BUILD_TYPE=Release',
               '-DCMAKE_INSTALL_PREFIX=/usr', '-DCMAKE_SKIP_RPATH=ON']
    if sdk:
        command.append('-DCMAKE_PREFIX_PATH=' + str(sdk.resolve()))
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(folder), '--target', 'kcalc', '--parallel', str(jobs)], check=True)
    return folder / 'bin/kcalc'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/kcalc-build')
    parser.add_argument('--sdk', type=Path)
    args = parser.parse_args()
    print(build(args.build_dir.resolve(), args.sdk))
