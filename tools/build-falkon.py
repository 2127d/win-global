#!/usr/bin/env python3
"""Build the native Falkon GUI fork; stage resources for its project launcher."""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'apps/falkon'


def build(folder, sdks=(), node=None):
    folder = folder.resolve()
    node = node or shutil.which('node') or ROOT / '.staging/node-v24.21.0-linux-x64/bin/node'
    node = Path(node).resolve()
    if not node.is_file():
        raise SystemExit('Install Node.js 22+ or pass --node /path/to/node for uBlock filtering.')
    major = int(subprocess.check_output([str(node), '-p', 'process.versions.node.split(".")[0]'], text=True))
    if major < 22:
        raise SystemExit('uBlock filtering requires Node.js 22 or newer.')
    command = ['cmake', '-S', str(SOURCE), '-B', str(folder), '-G', 'Ninja',
               '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_TESTING=OFF',
               '-DBUILD_PYTHON_SUPPORT=OFF', '-DBUILD_KEYRING=OFF',
               '-DCMAKE_INSTALL_PREFIX=' + str(folder / 'install')]
    if sdks:
        prefixes = ';'.join(str(p.resolve()) for p in sdks)
        command += ['-DCMAKE_PREFIX_PATH=' + prefixes, '-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=' + prefixes]
        for sdk in sdks:
            for package in (sdk / 'lib64/cmake').glob('Qt6*/'):
                if not (package / (package.name + 'Config.cmake')).is_file():
                    continue
                # Each explicit package comes from the first SDK containing it.
                arg = '-D' + package.name + '_DIR='
                if not any(value.startswith(arg) for value in command): command.append(arg + str(package.resolve()))
    subprocess.run(command, check=True)
    subprocess.run(['cmake', '--build', str(folder), '-j', '4'], check=True)
    resource = folder / 'share/falkon/themes'
    resource.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(SOURCE / 'themes', resource, dirs_exist_ok=True)
    shutil.copytree(SOURCE / 'resources/ublock', resource.parent / 'ublock', dirs_exist_ok=True)
    target_node = folder / 'bin/node'
    if node != target_node:
        shutil.copy2(node, target_node)
    # Retain the upstream runtime notices alongside the staged binary.
    license_file = node.parent.parent / 'LICENSE'
    if license_file.is_file():
        shutil.copy2(license_file, folder / 'bin/NODE-LICENSE')
    print(f'Built {folder / "bin/falkon"}')
    return folder / 'bin/falkon'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/falkon-native-build')
    parser.add_argument('--sdk', type=Path, action='append', default=[], help='Optional development prefix; repeat for multiple SDKs.')
    parser.add_argument('--node', type=Path, help='Node.js 22+ executable to stage with the browser.')
    args = parser.parse_args()
    build(args.build_dir, args.sdk, args.node)
