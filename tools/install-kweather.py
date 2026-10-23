#!/usr/bin/env python3
"""Install/remove Sequoia Weather without replacing its Flatpak or user settings."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import tempfile
from theme_files import Receipt
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('weather_build',ROOT/'tools/build-kweather.py')
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--prefix',type=Path,default=Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share'))
    p.add_argument('--bin-prefix',type=Path,default=Path.home()/'.local/bin')
    p.add_argument('--build-dir',type=Path,default=ROOT/'.staging/kweather-release')
    p.add_argument('--sdk',type=Path);p.add_argument('--skip-build',action='store_true')
    p.add_argument('--remove',action='store_true');p.add_argument('--check',action='store_true');p.add_argument('--force',action='store_true')
    args=p.parse_args()
    if args.check and not args.remove:p.error('--check requires --remove')
    data,bins=args.prefix.absolute(),args.bin_prefix.absolute();root=Path(os.path.commonpath([data,bins]))
    target=data.relative_to(root)/'sequoia-eleven/kweather'
    launcher=bins.relative_to(root)/'kweather'
    desktop=data.relative_to(root)/'applications/org.kde.kweather.desktop'
    def allowed(name):
        path=Path(name)
        return name in (str(launcher),str(desktop)) or (path.is_relative_to(target) and '..' not in path.parts and not path.is_relative_to(target/'installation'))
    receipt=Receipt(root,str(target/'installation'),allowed);receipt.validate(force=args.force)
    if args.check:return
    if args.remove:
        receipt.uninstall(force=args.force);print('Removed Sequoia Weather; restored the previous launcher. Locations and settings retained.');return
    stock=builder.stock()
    prefix=args.build_dir.resolve()/'install' if args.skip_build else builder.build(args.build_dir,args.sdk)
    if not (prefix/'bin/kweather').is_file():raise RuntimeError('Build Sequoia Weather first.')
    with tempfile.TemporaryDirectory(prefix='sequoia-weather-install-') as temp:
        stage=Path(temp);payload=stage/target;payload.mkdir(parents=True)
        # Runtime files only; development headers and CMake metadata stay in the build tree.
        for path in prefix.rglob('*'):
            rel=path.relative_to(prefix)
            if not path.is_file() or rel.parts[0] not in ('bin','lib','share'):continue
            if 'cmake' in rel.parts or rel.suffix in ('.a','.prl'):continue
            if rel.parts[0]=='share' and rel.parts[1] not in ('locale','icons'):continue
            dest=payload/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest,follow_symlinks=True)
        digests={str(f.relative_to(payload)):hashlib.sha256(f.read_bytes()).hexdigest() for f in payload.rglob('*') if f.is_file()}
        (payload/'build.json').write_text(json.dumps(dict(version=builder.VERSION,upstream_sha256=hashlib.sha256(stock.read_bytes()).hexdigest(),payload=digests),indent=2)+'\n')
        out=stage/launcher;out.parent.mkdir(parents=True,exist_ok=True)
        out.write_text((ROOT/'apps/kweather/launch.py').read_text().replace("'__SEQUOIA_KWEATHER_DATA__'",repr(str(data))));out.chmod(0o755)
        original=stock.parent.parent/'share/applications/org.kde.kweather.desktop'
        text=original.read_text();lines=[]
        executable=str(root/launcher).replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')
        for line in text.splitlines():
            if line.startswith('Exec='):line='Exec="'+executable+'"'
            if line.startswith('DBusActivatable='):line='DBusActivatable=false'
            if line.startswith('X-Flatpak=') or line.startswith('X-Flatpak-RenamedFrom='):continue
            lines.append(line)
        out=stage/desktop;out.parent.mkdir(parents=True,exist_ok=True);out.write_text('\n'.join(lines)+'\n')
        receipt.install(stage,force=args.force)
    # Import only on a real user install, and never replace native preferences.
    if data==Path(os.environ.get('XDG_DATA_HOME') or Path.home()/'.local/share'):
        flat=Path.home()/'.var/app/org.kde.kweather'
        config=Path(os.environ.get('XDG_CONFIG_HOME') or Path.home()/'.config')/'kweather/kweatherrc'
        if not config.exists() and (flat/'config/kweather/kweatherrc').is_file():
            config.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(flat/'config/kweather/kweatherrc',config)
        cache=Path(os.environ.get('XDG_CACHE_HOME') or Path.home()/'.cache')/'kweather'
        if not cache.exists() and (flat/'cache/kweather').is_dir():shutil.copytree(flat/'cache/kweather',cache)
    print(f'Installed {root/launcher}. Open Weather to use the Sequoia makeover.')

if __name__=='__main__':main()
