#!/usr/bin/env python3
"""Install or remove the QRCA presentation and both launchers reversibly."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import tempfile
from theme_files import Receipt
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('qrca_build', ROOT/'tools/build-qrca.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share'))
    parser.add_argument('--bin-prefix', type=Path, default=Path.home()/'.local/bin')
    parser.add_argument('--build-dir', type=Path, default=ROOT/'apps/qrca/.build')
    parser.add_argument('--sdk', type=Path)
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--remove', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args()
    if args.check and not args.remove: parser.error('--check requires --remove')
    data, bins = args.prefix.absolute(), args.bin_prefix.absolute()
    root = Path(os.path.commonpath([data,bins]))
    target = data.relative_to(root)/'sequoia-eleven/qrca'
    launcher = bins.relative_to(root)/'qrca'
    desktops = [data.relative_to(root)/'applications'/name for name in ('org.kde.qrca.desktop','org.kde.qrca.wifi.desktop')]
    def allowed(name):
        p = Path(name)
        return p == launcher or p in desktops or p in (target/'qrca',target/'build.json') or p.is_relative_to(target/'qml')
    receipt = Receipt(root, str(target/'installation'), allowed)
    receipt.validate(force=args.force)
    if args.check: return
    if args.remove:
        receipt.uninstall(force=args.force)
        print('Removed the QRCA makeover and restored both previous launchers.')
        return
    if builder.installed_version() != builder.VERSION:
        raise RuntimeError('Installed QRCA differs from the pinned source version.')
    folder = args.build_dir.resolve()
    if not args.skip_build: builder.build(folder,args.sdk)
    # Test instrumentation is never shipped in the installed app.
    cache=(folder/'CMakeCache.txt').read_text()
    if 'SEQUOIA_QRCA_VERIFY:UNINITIALIZED=ON' in cache or 'SEQUOIA_QRCA_VERIFY:BOOL=ON' in cache:
        raise RuntimeError('Build without --verify before installing.')
    digest=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    with tempfile.TemporaryDirectory(prefix='sequoia-qrca-install-') as temporary:
        stage=Path(temporary);runtime=stage/target;runtime.mkdir(parents=True)
        shutil.copy2(folder/'bin/qrca',runtime/'qrca')
        shutil.copytree(folder/'bin/qml',runtime/'qml')
        files={str(p.relative_to(runtime)):digest(p) for p in runtime.rglob('*') if p.is_file()}
        (runtime/'build.json').write_text(json.dumps({'version':builder.VERSION,'upstream_sha256':digest(Path('/usr/bin/qrca')),'files':files},indent=2)+'\n')
        launch=stage/launcher;launch.parent.mkdir(parents=True,exist_ok=True)
        launch.write_text((ROOT/'apps/qrca/launch.py').read_text().replace("'__SEQUOIA_QRCA_DATA__'",repr(str(data))))
        launch.chmod(0o755)
        executable=str(root/launcher).replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')
        for desktop in desktops:
            lines=(Path('/usr/share/applications')/desktop.name).read_text().splitlines()
            main=False
            for i,line in enumerate(lines):
                if line.startswith('['): main=line=='[Desktop Entry]'
                if main and line.startswith('Exec='):
                    lines[i]='Exec="'+executable+'"'+(' --wifi' if '.wifi.' in desktop.name else '')
            dest=stage/desktop;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text('\n'.join(lines)+'\n')
        receipt.install(stage,force=args.force)
    print('Installed QRCA makeover: '+str(root/launcher))

if __name__=='__main__':main()
