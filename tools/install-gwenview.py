#!/usr/bin/env python3
"""Install or remove the private Gwenview override for this account, reversibly."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from theme_files import Receipt
from installer_platform import gwenview_library

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('gwenview_build', ROOT / 'tools/build-gwenview.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home() / '.local/share'))
    parser.add_argument('--bin-prefix', type=Path, default=Path.home() / '.local/bin')
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.staging/gwenview-build')
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--remove', action='store_true')
    parser.add_argument('--check', action='store_true', help='Validate removal without changing files.')
    parser.add_argument('--force', action='store_true', help='Replace/remove edited managed files.')
    args = parser.parse_args()
    if args.check and not args.remove:
        parser.error('--check requires --remove')
    data, bins = args.prefix.absolute(), args.bin_prefix.absolute()
    root = Path(os.path.commonpath([data, bins]))
    target = data.relative_to(root) / 'sequoia-eleven/gwenview'
    launcher = bins.relative_to(root) / 'gwenview'
    desktop = data.relative_to(root) / 'applications/org.kde.gwenview.desktop'
    allowed = {str(launcher), str(desktop), *(str(target / name) for name in ('gwenview', 'libgwenviewlib.so.5', 'build.json'))}
    receipt = Receipt(root, str(target / 'installation'), allowed.__contains__)
    receipt.validate(force=args.force)
    if args.check:
        return
    if args.remove:
        receipt.uninstall(force=args.force)
        print('Removed the Gwenview override and restored the previous launcher.')
        return
    if builder.installed_version() != builder.VERSION:
        raise RuntimeError('Installed Gwenview differs from this source version.')
    binary = args.build_dir.resolve() / 'bin/gwenview'
    if not args.skip_build: binary = builder.build(args.build_dir.resolve(), args.sdk)
    library = binary.parent / 'libgwenviewlib.so.5'
    for file in (binary, library):
        dynamic = subprocess.check_output(['readelf', '-d', str(file)], text=True)
        if not any('Library runpath: [$ORIGIN]' in line or 'Library rpath: [$ORIGIN]' in line for line in dynamic.splitlines()):
            raise RuntimeError(f'{file} must use the private $ORIGIN library path; rebuild with build-gwenview.py.')
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    with tempfile.TemporaryDirectory(prefix='sequoia-gwenview-install-') as temp:
        stage = Path(temp)
        (stage / target).mkdir(parents=True)
        (stage / launcher).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, stage / target / 'gwenview')
        shutil.copy2(library, stage / target / 'libgwenviewlib.so.5')
        (stage / target / 'build.json').write_text(json.dumps({
            'version': builder.VERSION,
            'upstream_sha256': digest(Path('/usr/bin/gwenview')),
            'upstream_library_sha256': digest(gwenview_library()),
            'upstream_library': str(gwenview_library()),
            'binary_sha256': digest(binary), 'library_sha256': digest(library),
        }, indent=2) + '\n')
        source = (ROOT / 'apps/gwenview/launch.py').read_text().replace("'__SEQUOIA_GWENVIEW_DATA__'", repr(str(data)))
        source = source.replace("Path('/usr/lib64/libgwenviewlib.so.5')", 'Path(' + repr(str(gwenview_library())) + ')')
        (stage / launcher).write_text(source)
        (stage / launcher).chmod(0o755)
        desktop_source = (root / desktop) if (root / desktop).is_file() else Path('/usr/share/applications/org.kde.gwenview.desktop')
        desktop_text = desktop_source.read_text()
        executable = str(root / launcher).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$')
        lines = desktop_text.splitlines()
        in_main = False
        for index, line in enumerate(lines):
            if line.startswith('['): in_main = line == '[Desktop Entry]'
            if in_main and line.startswith('Exec='): lines[index] = 'Exec="' + executable + '" %U'
        (stage / desktop).parent.mkdir(parents=True, exist_ok=True)
        (stage / desktop).write_text('\n'.join(lines) + '\n')
        env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
        env.pop('LD_PRELOAD', None)
        env['XDG_CONFIG_HOME'] = str(stage / 'config')
        env['DBUS_SESSION_BUS_ADDRESS'] = 'unix:path=/nonexistent-gwenview-version-check'
        result = subprocess.check_output([str(stage / target / 'gwenview'), '--version'], env=env, text=True, timeout=15)
        if result.strip().split()[-1] != builder.VERSION: raise RuntimeError('Unexpected built version.')
        receipt.install(stage, force=args.force)
    print(f'Installed: {root / launcher}. Close and reopen Gwenview to use the new interface.')


if __name__ == '__main__': main()
