#!/usr/bin/env python3
"""Build the Gwenview presentation override with the installed KDE libraries."""
import argparse
from pathlib import Path
import subprocess
from installer_platform import application_version, build_jobs

ROOT = Path(__file__).resolve().parents[1]
VERSION = '26.08.1'


def installed_version():
    return application_version('gwenview')


def build(folder, sdk=None):
    jobs = build_jobs()
    print(f'Building Gwenview with {jobs} parallel jobs.', flush=True)
    if installed_version() != VERSION:
        raise RuntimeError('Update the Gwenview source fork to match the installed version first.')
    # Rediscover dependencies instead of retaining a previous SDK's paths.
    command = ['cmake', '--fresh', '-S', str(ROOT / f'apps/gwenview/gwenview-{VERSION}'), '-B', str(folder),
               '-G', 'Ninja', '-DBUILD_TESTING=OFF', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_INSTALL_PREFIX=/usr',
               '-DKDE_SKIP_RPATH_SETTINGS=ON', '-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON', '-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF', '-DCMAKE_INSTALL_RPATH=$ORIGIN']
    if sdk:
        sdk = sdk.resolve()
        command += ['-DCMAKE_PREFIX_PATH=' + str(sdk), '-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=' + str(sdk)]
        for component in ('Svg', 'Multimedia', 'MultimediaWidgets'):
            package = sdk / f'lib64/cmake/Qt6{component}'
            if package.is_dir(): command.append(f'-DQt6{component}_DIR={package}')
        if (sdk / 'include/cfitsio').is_dir():
            command.append('-DCFITSIO_INCLUDE_DIR=' + str(sdk / 'include/cfitsio'))
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(folder), '--target', 'gwenview', '--parallel', str(jobs)], check=True)
    return folder / 'bin/gwenview'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/gwenview-build')
    parser.add_argument('--sdk', type=Path)
    args = parser.parse_args()
    print(build(args.build_dir.resolve(), args.sdk))
