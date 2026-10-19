#!/usr/bin/env python3
"""Build the native Plasma Login presentation bridge, without installing it."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GREETER = Path('/usr/libexec/plasma-login-greeter')
INSTALL_DIR = Path('/usr/local/share/sequoia-eleven/login-screen')


def build(output, runtime=None):
    if not GREETER.is_file():
        raise RuntimeError('This login theme requires Plasma Login Manager (SDDM is not supported).')
    output.mkdir(parents=True, exist_ok=True)
    runtime = runtime or INSTALL_DIR
    for name in ('qml', 'assets'):
        shutil.copytree(ROOT/'themes/login-screen'/name, output/name, dirs_exist_ok=True)
    shutil.copy2(ROOT/'themes/wallpapers/sequoia-forest.jpg', output/'assets/sequoia-forest.jpg')
    digest = hashlib.sha256(GREETER.read_bytes()).hexdigest()
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Core', 'Qt6Quick', 'Qt6Qml'], text=True))
    with tempfile.TemporaryDirectory(prefix='sequoia-login-build-') as temporary:
        generated = Path(temporary)
        libexec = subprocess.check_output(['pkg-config', '--variable=libexecdir', 'Qt6Core'], text=True).strip()
        subprocess.run([str(Path(libexec) / 'moc'), str(ROOT/'themes/login-screen/native/wallpaper-glass.cpp'),
                        '-o', str(generated/'wallpaper-glass.moc'),
                        *(flag for flag in flags if flag.startswith(('-I', '-D')))], check=True)
        subprocess.run(['c++', '-std=c++17', '-O2', '-fPIC', '-shared',
            str(ROOT/'themes/login-screen/native/bridge.cpp'), str(ROOT/'themes/login-screen/native/wallpaper-glass.cpp'),
            '-I' + str(generated), '-o', str(output/'bridge.so'),
            '-DSEQUOIA_LOGIN_MAIN=' + json.dumps(str(runtime/'qml/Main.qml')),
            '-DSEQUOIA_LOGIN_SHA256=' + json.dumps(digest), *flags, '-ldl'], check=True)
    (output/'compatibility.json').write_text(json.dumps({'greeter': str(GREETER), 'sha256': digest}, indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--runtime', type=Path, help='Override installed path for an isolated preview.')
    args = parser.parse_args()
    build(args.output.resolve(), args.runtime.resolve() if args.runtime else None)
